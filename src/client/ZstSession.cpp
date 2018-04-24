#include "ZstSession.h"

ZstSession::ZstSession(ZstClient * client) : ZstClientModule(client)
{
	m_hierarchy = new ZstHierarchy(client);
}

ZstSession::~ZstSession() {
	delete m_hierarchy;
	delete m_reaper;
}


// ------------------------------
// Delegator overrides
// ------------------------------

void ZstSession::init()
{
	m_hierarchy->init(client_name);
}

void ZstSession::destroy()
{
	m_hierarchy->destroy();
}

void ZstSession::process_events()
{
	//Process queued hierarchy change updates
	m_hierarchy->process_events();
	
	//Process queued performance updates
	ZstEventDispatcher<ZstSessionAdaptor*>::process_events();
	ZstEventDispatcher<ZstStageAdaptor*>::process_events();
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::process_events();

	m_reaper->reap_all();
}

void ZstSession::flush()
{
	ZstEventDispatcher<ZstSessionAdaptor*>::flush();
	ZstEventDispatcher<ZstStageAdaptor*>::flush();
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::flush();
	m_hierarchy->flush();
}


// ---------------------------
// Adaptor plug send/receive
// ---------------------------

void ZstSession::on_receive_from_performance(ZstPerformanceMessage * msg)
{
	ZstURI sender((char*)msg->payload_at(0).data(), msg->payload_at(0).size());

	//Find local proxy for the sending plug
	ZstPlug * sending_plug = dynamic_cast<ZstPlug*>(m_hierarchy->find_entity(sender));
	ZstInputPlug * receiving_plug = NULL;

	if (!sending_plug) {
		ZstLog::net(LogLevel::warn, "No sending plug found");
	}

	//Iterate over all connected cables from the sending plug
	for (auto cable : *sending_plug) {
		receiving_plug = dynamic_cast<ZstInputPlug*>(cable->get_input());
		if (receiving_plug) {
			if (m_hierarchy->entity_is_local(*receiving_plug)) {
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				this->plug_raw_value(receiving_plug)->read((char*)msg->payload_at(1).data(), msg->payload_at(1).size(), offset);
				add_event([receiving_plug](ZstSessionAdaptor * dlg) {dlg->on_plug_received_value(receiving_plug); });
			}
		}
	}
}

void ZstSession::on_plug_fire(ZstOutputPlug * plug)
{
	run_event([plug](ZstSessionAdaptor * dlg) {
		dlg->send_to_performance(client()->msg_dispatch()->init_performance_message(plug));
	});
}

void ZstSession::on_plug_received_value(ZstInputPlug * plug)
{
	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	if (!parent) {
		throw std::runtime_error("Could not find parent of input plug");
	}
	try {
		parent->compute(plug);
	}
	catch (std::exception e) {
		ZstLog::entity(LogLevel::error, "Compute on component {} failed. Error was: {}", parent->URI().path(), e.what());
	}
}

void ZstSession::on_receive_from_stage(int payload_index, ZstStageMessage * msg)
{
	switch (msg->payload_at(payload_index).kind()) {
	case ZstMsgKind::CREATE_CABLE:
	{
		const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(payload_index);
		create_cable(cable);
		break;
	}
	case ZstMsgKind::DESTROY_CABLE:
	{
		const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(payload_index);
		ZstCable * cable_ptr = find_cable(cable.get_input_URI(), cable.get_output_URI());
		if (cable_ptr) {
			destroy_cable_complete(ZstMessageReceipt{ ZstMsgKind::OK, false }, cable_ptr);
		}
		break;
	}
	default:
		ZstLog::net(LogLevel::notification, "Didn't understand message type of {}", msg->payload_at(payload_index).kind());
		throw std::logic_error("Didn't understand message type");
		break;
	}
}

void ZstSession::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	if(synchronisable->is_proxy())
		m_reaper->add(synchronisable);
}


// ------------------
// Cable creation API
// ------------------

ZstCable * ZstSession::connect_cable(ZstPlug * input, ZstPlug * output, bool async)
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

	cable = create_cable(input, output);
	if (!cable) {
		ZstLog::net(LogLevel::notification, "Couldn't create cable, already exists!");
		return NULL;
	}

	synchronisable_set_activating(cable);

	//If either of the cable plugs are a local entity, then the cable is local as well
	if (m_hierarchy->entity_is_local(*input) || m_hierarchy->entity_is_local(*output)) {
		cable_set_local(cable);
	}

	//TODO: Even though we use a cable object when sending over the wire, it's up to us
	//to determine the correct input->output order - fix this using ZstInputPlug and 
	//ZstOutput plug as arguments
	ZstStageMessage * msg = client()->msg_dispatch()->init_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable);
	run_event([this, msg, async, cable](ZstStageAdaptor * adaptor) {
		ZstMessageReceipt response = adaptor->send_to_stage(msg, async, [this, cable](ZstMessageReceipt response) {
			this->connect_cable_complete(response, cable);
		});
	});

	//Create the cable early so we have something to return immediately
	return cable;
}

void ZstSession::connect_cable_complete(ZstMessageReceipt response, ZstCable * cable) {
	if (response.status == ZstMsgKind::OK) {
		synchronisable_enqueue_activation(cable);
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), response.status);
	}
}

void ZstSession::destroy_cable(ZstCable * cable, bool async)
{
	//Need to set this cable as deactivating so the stage publish message doesn't clean it up too early
	synchronisable_set_deactivating(cable);

	ZstStageMessage * msg = client()->msg_dispatch()->init_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable);
	client()->msg_dispatch()->send_to_stage(msg, async, [this, cable](ZstMessageReceipt response) { this->destroy_cable_complete(response, cable); });
}

void ZstSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
	if (!cable || !cable->is_activated())
		return;

	//Remove cable from local list so that other threads don't assume it still exists
	m_cables.erase(cable);

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy cable failed with status {}", response.status);
	}
	ZstLog::net(LogLevel::notification, "Destroy cable completed with status {}", response.status);

	//Find the plugs and disconnect them seperately, in case they have already disappeared
	ZstPlug * input = dynamic_cast<ZstPlug*>(m_hierarchy->find_entity(cable->get_input_URI()));
	ZstPlug * output = dynamic_cast<ZstPlug*>(m_hierarchy->find_entity(cable->get_output_URI()));

	if (input)
		plug_remove_cable(input, cable);

	if (output)
		plug_remove_cable(output, cable);

	cable->set_input(NULL);
	cable->set_output(NULL);

	//Queue events
	synchronisable_enqueue_deactivation(cable);
	add_event([cable](ZstSessionAdaptor * dlg) { dlg->on_cable_destroyed(cable); });
}

void ZstSession::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}


// -------------
// Cable queries
// -------------

ZstCable * ZstSession::find_cable(const ZstURI & input_path, const ZstURI & output_path)
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

ZstCable * ZstSession::find_cable(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return find_cable(input->URI(), output->URI());
}


// -----------------------------
// Cable creation implementation
// -----------------------------

ZstCable * ZstSession::create_cable(const ZstCable & cable)
{
	return create_cable(cable.get_input_URI(), cable.get_output_URI());
}

ZstCable * ZstSession::create_cable(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return create_cable(input->URI(), output->URI());
}

ZstCable * ZstSession::create_cable(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable_ptr = find_cable(input_path, output_path);
	if (cable_ptr) {
		return cable_ptr;
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
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(m_hierarchy->find_entity(input_path));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(m_hierarchy->find_entity(output_path));
	plug_add_cable(input_plug, cable_ptr);
	plug_add_cable(output_plug, cable_ptr);
	cable_ptr->set_input(input_plug);
	cable_ptr->set_output(output_plug);

	//Set adaptors
	cable_ptr->add_adaptor(this);

	//Cables are always local so they can be cleaned up by the reaper
	synchronisable_set_proxy(cable_ptr);

	//Enqueue events
	synchronisable_set_activation_status(cable_ptr, ZstSyncStatus::ACTIVATED);

	add_event([cable_ptr](ZstSessionAdaptor * dlg) { dlg->on_cable_created(cable_ptr); });
	return cable_ptr;
}
