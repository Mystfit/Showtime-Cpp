#include "ZstCableNetwork.h"



ZstCableNetwork::ZstCableNetwork()
{
}


ZstCableNetwork::~ZstCableNetwork()
{
}


ZstCable * ZstCableNetwork::connect_cable(ZstPlug * input, ZstPlug * output, bool async)
{
	ZstCable * cable = NULL;

	if (!input || !output) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug missing.");
		return cable;
	}

	if (!input->is_activated() || !output->is_activated()) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug is not activated.");
		return cable;
	}

	if (input->direction() != ZstPlugDirection::IN_JACK || output->direction() != ZstPlugDirection::OUT_JACK) {
		ZstLog::net(LogLevel::notification, "Cable order incorrect");
		return NULL;
	}

	cable = create_cable_ptr(input, output);
	if (!cable) {
		ZstLog::net(LogLevel::notification, "Couldn't create cable, already exists!");
		return NULL;
	}

	cable->set_activating();

	//If either of the cable plugs are a local entity, then the cable is local as well
	if (entity_is_local(*input) || entity_is_local(*output)) {
		cable->set_local();
	}

	//TODO: Even though we use a cable object when sending over the wire, it's up to us
	//to determine the correct input->output order - fix this using ZstInputPlug and 
	//ZstOutput plug as arguments
	ZstMessage * msg = msg_pool().get()->init_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable);
	MessageFuture future = msg_pool().register_response_message(msg, true);

	if (async) {
		connect_cable_async(cable, future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		connect_cable_sync(cable, future);
	}

	//Create the cable early so we have something to return immediately
	return cable;
}


void ZstCableNetwork::destroy_cable(ZstCable * cable, bool async)
{
	//Need to set this cable as deactivating so the stage publish message doesn't clean it up too early
	cable->set_deactivating();
	ZstMessage * msg = msg_pool().get()->init_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable);

	MessageFuture future = msg_pool().register_response_message(msg, true);
	try {
		if (async) {
			destroy_cable_async(cable, future);
			send_to_stage(msg);
		}
		else {
			send_to_stage(msg);
			destroy_cable_sync(cable, future);
		}
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Destroy cable timed out: {}", e.what());
	}
}


void ZstCableNetwork::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable_ptr(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}


void ZstCableNetwork::connect_cable_complete(ZstMsgKind status, ZstCable * cable) {
	if (status == ZstMsgKind::OK) {
		cable->enqueue_activation();
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), status);
	}
}


void ZstCableNetwork::destroy_cable_complete(ZstMsgKind status, ZstCable * cable)
{
	if (!cable)
		return;

	//Remove cable from local list so that other threads don't assume it still exists
	m_cables.erase(cable);

	if (status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy cable failed with status {}", status);
	}
	ZstLog::net(LogLevel::notification, "Destroy cable completed with status {}", status);

	//Find the plugs and disconnect them seperately, in case they have already disappeared
	ZstPlug * input = dynamic_cast<ZstPlug*>(find_entity(cable->get_input_URI()));
	ZstPlug * output = dynamic_cast<ZstPlug*>(find_entity(cable->get_output_URI()));

	if (input)
		input->remove_cable(cable);

	if (output)
		output->remove_cable(cable);

	cable->set_input(NULL);
	cable->set_output(NULL);

	m_reaper.add(cable);
}



ZstCable * ZstCableNetwork::find_cable(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable = NULL;
	if (m_cables.size()) {
		auto search_cable = ZstCable(input_path, output_path);
		auto cable_ptr = m_cables.find(&search_cable);
		if (cable_ptr != m_cables.end()) {
			cable = *cable_ptr;
		}
	}
	return cable;
}

ZstCable * ZstCableNetwork::find_cable(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return find_cable(input->URI(), output->URI());
}


ZstEventQueue & ZstCableNetwork::cable_arriving_events()
{
	return m_cable_arriving_event_manager;
}

ZstEventQueue & ZstCableNetwork::cable_leaving_events()
{
	return m_cable_leaving_event_manager;
}


ZstCable * ZstCableNetwork::create_cable_ptr(const ZstCable & cable)
{
	return create_cable_ptr(cable.get_input_URI(), cable.get_output_URI());
}

ZstCable * ZstCableNetwork::create_cable_ptr(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return create_cable_ptr(input->URI(), output->URI());
}

ZstCable * ZstCableNetwork::create_cable_ptr(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable_ptr = find_cable_ptr(input_path, output_path);
	if (cable_ptr) {
		return NULL;
	}

	//Create and store new cable
	cable_ptr = ZstCable::create(input_path, output_path);
	try {
		m_cables.insert(cable_ptr);
	}
	catch (std::exception e) {
		ZstLog::net(LogLevel::notification, "Couldn't insert cable. Reason:", e.what());
		ZstCable::destroy(cable_ptr);
		cable_ptr = NULL;
		return cable_ptr;
	}

	//Add cable to plugs
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(find_entity(input_path));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(find_entity(output_path));
	input_plug->add_cable(cable_ptr);
	output_plug->add_cable(cable_ptr);
	cable_ptr->set_input(input_plug);
	cable_ptr->set_output(output_plug);

	//Set network interactor
	cable_ptr->set_network_interactor(this);

	return cable_ptr;
}
