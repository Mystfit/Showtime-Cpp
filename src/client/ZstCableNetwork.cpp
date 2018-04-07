#include "ZstCableNetwork.h"

ZstCableNetwork::ZstCableNetwork(ZstClient * client) : 
	ZstClientModule(client)
{
	m_cable_arriving_event_manager = new ZstEventQueue();
	add_event_queue(m_cable_arriving_event_manager);
	
	m_cable_leaving_hook = new ZstCableLeavingEvent();
	m_cable_leaving_event_manager = new ZstEventQueue();
	m_cable_leaving_event_manager->attach_post_event_callback(m_cable_leaving_hook);
	add_event_queue(m_cable_leaving_event_manager);
}

ZstCableNetwork::~ZstCableNetwork()
{
	m_cable_leaving_event_manager->remove_post_event_callback(m_cable_leaving_hook);
	remove_event_queue(m_cable_leaving_event_manager);
	remove_event_queue(m_cable_arriving_event_manager);

	delete m_cable_leaving_event_manager;
	delete m_cable_leaving_hook;
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
	if (client()->hierarchy()->entity_is_local(*input) || client()->hierarchy()->entity_is_local(*output)) {
		cable->set_local();
	}

	//TODO: Even though we use a cable object when sending over the wire, it's up to us
	//to determine the correct input->output order - fix this using ZstInputPlug and 
	//ZstOutput plug as arguments
	ZstMessage * msg = client()->msg_dispatch()->init_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable);
	client()->msg_dispatch()->send_to_stage(msg, [this, cable](ZstMessageReceipt response) { this->connect_cable_complete(response, cable); }, async);

	//Create the cable early so we have something to return immediately
	return cable;
}

void ZstCableNetwork::connect_cable_complete(ZstMessageReceipt response, ZstCable * cable) {
	if (response.status == ZstMsgKind::OK) {
		cable->enqueue_activation();
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), status);
	}
}

void ZstCableNetwork::destroy_cable(ZstCable * cable, bool async)
{
	//Need to set this cable as deactivating so the stage publish message doesn't clean it up too early
	cable->set_deactivating();
	ZstMessage * msg = client()->msg_dispatch()->init_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable);
	client()->msg_dispatch()->send_to_stage(msg, [this, cable](ZstMessageReceipt response) { this->destroy_cable_complete(response, cable); }, async); 
}

void ZstCableNetwork::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
	if (!cable)
		return;

	//Remove cable from local list so that other threads don't assume it still exists
	m_cables.erase(cable);

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy cable failed with status {}", response.status);
	}
	ZstLog::net(LogLevel::notification, "Destroy cable completed with status {}", response.status);

	//Find the plugs and disconnect them seperately, in case they have already disappeared
	ZstPlug * input = dynamic_cast<ZstPlug*>(client()->hierarchy()->find_entity(cable->get_input_URI()));
	ZstPlug * output = dynamic_cast<ZstPlug*>(client()->hierarchy()->find_entity(cable->get_output_URI()));

	if (input)
		input->remove_cable(cable);

	if (output)
		output->remove_cable(cable);

	cable->set_input(NULL);
	cable->set_output(NULL);

	client()->enqueue_synchronisable_deletion(cable);
}

void ZstCableNetwork::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
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


ZstEventQueue * ZstCableNetwork::cable_arriving_events()
{
	return m_cable_arriving_event_manager;
}

ZstEventQueue * ZstCableNetwork::cable_leaving_events()
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
	ZstCable * cable_ptr = find_cable(input_path, output_path);
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
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(client()->hierarchy()->find_entity(input_path));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(client()->hierarchy()->find_entity(output_path));
	input_plug->add_cable(cable_ptr);
	output_plug->add_cable(cable_ptr);
	cable_ptr->set_input(input_plug);
	cable_ptr->set_output(output_plug);

	//Set network interactor
	cable_ptr->set_network_interactor(client());

	return cable_ptr;
}


// ---------------------------
// Sync/async block to convert
// ---------------------------

//void ZstClient::connect_cable_sync(ZstCable * cable, MessageFuture & future)
//{
//	try {
//		ZstMsgKind status = future.get();
//		connect_cable_complete(status, cable);
//		process_callbacks();
//	}
//	catch (const ZstTimeoutException & e) {
//		ZstLog::net(LogLevel::notification, "Connect cable sync timed out: {}", e.what());
//	}
//}
//
//void ZstClient::connect_cable_async(ZstCable * cable, MessageFuture & future)
//{
//	future.then([this, cable](MessageFuture f) {
//		ZstMsgKind status(ZstMsgKind::EMPTY);
//		try {
//			status = f.get();
//			this->connect_cable_complete(status, cable);
//		}
//		catch (const ZstTimeoutException & e) {
//			ZstLog::net(LogLevel::notification, "Connect cable async timed out: {}", e.what());
//			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
//		}
//		return status;
//	});
//}
//
//
//
//void ZstClient::destroy_cable_sync(ZstCable * cable, MessageFuture & future)
//{
//    ZstMsgKind status(ZstMsgKind::EMPTY);
//	try {
//        status = future.get();
//		cable->enqueue_deactivation();
//		cable_leaving_events().enqueue(cable);
//		process_callbacks();
//	}
//	catch (const ZstTimeoutException & e) {
//		ZstLog::net(LogLevel::notification, "Destroy cable sync timed out: ", e.what());
//	}
//}
//
//void ZstClient::destroy_cable_async(ZstCable * cable, MessageFuture & future)
//{
//	future.then([this, cable](MessageFuture f) {
//		ZstMsgKind status(ZstMsgKind::EMPTY);
//		try {
//			status = f.get();
//			cable->enqueue_deactivation();
//			this->cable_leaving_events().enqueue(cable);
//		}
//		catch (const ZstTimeoutException & e) {
//			ZstLog::net(LogLevel::notification, "Destroy cable async timed out: {}", e.what());
//			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
//		}
//		return status;
//	});
//}