#include "ZstClientSession.h"

ZstClientSession::ZstClientSession() : 
	m_stage_events("session stage"),
	m_performance_events("session performance"),
	m_synchronisable_events("session synchronisables")
{
	m_reaper = new ZstReaper();
	m_hierarchy = new ZstClientHierarchy();
}

ZstClientSession::~ZstClientSession() {
	delete m_reaper;
	delete m_hierarchy;
}


// ------------------------------
// Delegator overrides
// ------------------------------

void ZstClientSession::init(std::string client_name)
{
	m_hierarchy->init(client_name);

	//We add this instance as an adaptor to make sure we can process local queued events
	m_synchronisable_events.add_adaptor(this);
}

void ZstClientSession::destroy()
{
	m_hierarchy->destroy();
	delete m_hierarchy;
}

void ZstClientSession::process_events()
{
	ZstSession::process_events();
	m_stage_events.process_events();
	m_synchronisable_events.process_events();
	m_reaper->reap_all();
}

void ZstClientSession::flush()
{
	ZstSession::flush();
	m_synchronisable_events.flush();
	m_stage_events.flush();
}

void ZstClientSession::dispatch_connected_to_stage()
{
	//Activate all owned entities
	synchronisable_enqueue_activation(static_cast<ZstClientHierarchy*>(hierarchy())->get_local_performer());
	session_events().invoke([](ZstSessionAdaptor * adaptor) {
		adaptor->on_connected_to_stage();
	});
}

void ZstClientSession::dispatch_disconnected_from_stage()
{
	//Deactivate all owned entities
	ZstLog::net(LogLevel::debug, "!!!Deactivating performer {}", hierarchy()->get_local_performer()->URI().path());
	synchronisable_enqueue_deactivation(static_cast<ZstClientHierarchy*>(hierarchy())->get_local_performer());
	session_events().invoke([](ZstSessionAdaptor * adaptor) {
		adaptor->on_disconnected_from_stage();
	});
}


// ---------------------------
// Adaptor plug send/receive
// ---------------------------

void ZstClientSession::on_receive_from_performance(ZstMessage * msg)
{
	ZstURI sender((char*)msg->payload_at(0).data(), msg->payload_at(0).size());

	//Find local proxy for the sending plug
	ZstPlug * sending_plug = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(sender));
	ZstInputPlug * receiving_plug = NULL;

	if (!sending_plug) {
		ZstLog::net(LogLevel::warn, "No sending plug found");
	}

	//Iterate over all connected cables from the sending plug
	for (auto cable : *sending_plug) {
		receiving_plug = dynamic_cast<ZstInputPlug*>(cable->get_input());
		if (receiving_plug) {
			if (!receiving_plug->is_proxy()) {
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				this->plug_raw_value(receiving_plug)->read((char*)msg->payload_at(1).data(), msg->payload_at(1).size(), offset);
				plug_received_value(receiving_plug);
			}
		}
	}
}

void ZstClientSession::on_plug_fire(ZstOutputPlug * plug)
{
	m_performance_events.invoke([plug](ZstPerformanceDispatchAdaptor * adaptor) {
		adaptor->send_to_performance(plug);
	});
}

void ZstClientSession::plug_received_value(ZstInputPlug * plug)
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

void ZstClientSession::on_receive_from_stage(size_t payload_index, ZstMessage * msg)
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
		ZstLog::net(LogLevel::warn, "Session message handler didn't understand message type of {}", msg->payload_at(payload_index).kind());
		break;
	}
}

void ZstClientSession::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	if(synchronisable->is_proxy())
		m_reaper->add(synchronisable);
}

void ZstClientSession::synchronisable_has_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_events.defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
		this->synchronisable_process_events(synchronisable);
	});
}


// ------------------
// Cable creation API
// ------------------

ZstCable * ZstClientSession::connect_cable(ZstPlug * input, ZstPlug * output, bool async)
{
	ZstCable * cable = NULL;
	cable = ZstSession::connect_cable(input, output, async);

	if (cable) {
		//TODO: Even though we use a cable object when sending over the wire, it's up to us
		//to determine the correct input->output order - fix this using ZstInputPlug and 
		//ZstOutput plug as arguments
		m_stage_events.invoke([this, async, cable](ZstStageDispatchAdaptor* adaptor) {
			adaptor->send_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable, async, [this, cable](ZstMessageReceipt response) {
				this->connect_cable_complete(response, cable);
			});
		});
	}

	return cable;
}

void ZstClientSession::connect_cable_complete(ZstMessageReceipt response, ZstCable * cable) {
	if (response.status == ZstMsgKind::OK) {
		synchronisable_enqueue_activation(cable);
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), response.status);
	}
}

void ZstClientSession::destroy_cable(ZstCable * cable, bool async)
{
	ZstSession::destroy_cable(cable, async);
	
	m_stage_events.invoke([this, cable, async](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable, async, [this, cable](ZstMessageReceipt response) {
			this->destroy_cable_complete(response, cable);
		});
	});

	if (!async) process_events();
}

void ZstClientSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
	if (!cable) return;

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::notification, "Destroy cable failed with status {}", response.status);
	}
	ZstLog::net(LogLevel::notification, "Destroy cable completed with status {}", response.status);

	//Find the plugs and disconnect them seperately, in case they have already disappeared
	ZstPlug * input = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(cable->get_input_URI()));
	ZstPlug * output = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(cable->get_output_URI()));

	if (input)
		plug_remove_cable(input, cable);

	if (output)
		plug_remove_cable(output, cable);

	cable->set_input(NULL);
	cable->set_output(NULL);

	//Queue events
	ZstLog::net(LogLevel::debug, "!!!Deactivating cable {}--{}", cable->get_input_URI().path(), cable->get_output_URI().path());
	session_events().defer([cable](ZstSessionAdaptor * dlg) { dlg->on_cable_destroyed(cable); });
	synchronisable_enqueue_deactivation(cable);
}


// -----------------
// Event dispatchers
// -----------------

ZstEventDispatcher<ZstStageDispatchAdaptor*>& ZstClientSession::stage_events()
{
	return m_stage_events;
}

ZstEventDispatcher<ZstPerformanceDispatchAdaptor*>& ZstClientSession::performance_events()
{
	return m_performance_events;
}

ZstClientHierarchy * ZstClientSession::hierarchy()
{
	return m_hierarchy;
}
