#include "ZstClientSession.h"

ZstClientSession::ZstClientSession() : 
	m_stage_events("session stage"),
	m_performance_events("session performance")
{
	m_hierarchy = new ZstClientHierarchy();
}

ZstClientSession::~ZstClientSession() {
	delete m_hierarchy;
}


// ------------------------------
// Delegator overrides
// ------------------------------

void ZstClientSession::init(std::string client_name)
{
	m_hierarchy->init(client_name);
	ZstSession::init();
}

void ZstClientSession::destroy()
{
	ZstSession::destroy();
	m_hierarchy->destroy();
}

void ZstClientSession::process_events()
{
	ZstSession::process_events();
	m_stage_events.process_events();
	m_reaper.reap_all();
}

void ZstClientSession::flush()
{
	ZstSession::flush();
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
	synchronisable_enqueue_deactivation(static_cast<ZstClientHierarchy*>(hierarchy())->get_local_performer());
	session_events().invoke([](ZstSessionAdaptor * adaptor) {
		adaptor->on_disconnected_from_stage();
	});
}


// ---------------------------
// Adaptor plug send/receive
// ---------------------------

void ZstClientSession::on_receive_msg(ZstMessage * msg)
{
	switch (msg->kind()) {
	case ZstMsgKind::CREATE_CABLE:
	{
		const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
		create_cable(cable);
		break;
	}
	case ZstMsgKind::DESTROY_CABLE:
	{
		const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(0);
		ZstCable * cable_ptr = find_cable(cable.get_input_URI(), cable.get_output_URI());
		if (cable_ptr) {
			destroy_cable_complete(ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportSendType::SYNC_REPLY }, cable_ptr);
		}
		break;
	}
	case ZstMsgKind::PERFORMANCE_MSG:
		on_receive_graph_msg(msg);
		break;
	default:
		break;
	}
}

void ZstClientSession::on_receive_graph_msg(ZstMessage * msg)
{
	//Check for no payloads in message
	if(msg->num_payloads() < 1){
		return;
	}

	//Find local proxy for the sending plug
	ZstPlug * sending_plug = dynamic_cast<ZstPlug*>(hierarchy()->find_entity(ZstURI(msg->get_arg("path"))));
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
				receiving_plug->raw_value()->read((char*)msg->payload_at(0).data(), msg->payload_at(0).size(), offset);
				plug_received_value(receiving_plug);
			}
		}
	}
}

void ZstClientSession::on_plug_fire(ZstOutputPlug * plug)
{
	m_performance_events.invoke([plug](ZstTransportAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::PERFORMANCE_MSG, {{"path", plug->URI().path()}}, *plug->raw_value());
	});
}

void ZstClientSession::plug_received_value(ZstInputPlug * plug)
{
	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	if (!parent) {
		throw std::runtime_error("Could not find parent of input plug");
	}
	compute_events().defer([parent, plug](ZstComputeAdaptor * adaptor) {
		adaptor->on_compute(parent, plug);
	});
}


void ZstClientSession::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	if(synchronisable->is_proxy())
		m_reaper.add(synchronisable);
}

void ZstClientSession::synchronisable_has_event(ZstSynchronisable * synchronisable)
{
	synchronisable_events().defer([this, synchronisable](ZstSynchronisableAdaptor * dlg) {
		this->synchronisable_process_events(synchronisable);
	});
}


// ------------------
// Cable creation API
// ------------------

ZstCable * ZstClientSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportSendType & sendtype)
{
	ZstCable * cable = NULL;
	cable = ZstSession::connect_cable(input, output, sendtype);
	
	if (cable) {
		//TODO: Even though we use a cable object when sending over the wire, it's up to us
		//to determine the correct input->output order - fix this using ZstInputPlug and 
		//ZstOutput plug as arguments
		m_stage_events.invoke([this, sendtype, cable](ZstTransportAdaptor* adaptor) {
			adaptor->send_message(ZstMsgKind::CREATE_CABLE, sendtype, *cable, [this, cable](ZstMessageReceipt response) {
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
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), ZstMsgNames[response.status]);
	}
}

void ZstClientSession::destroy_cable(ZstCable * cable, const ZstTransportSendType & sendtype)
{
	ZstSession::destroy_cable(cable);
	
	m_stage_events.invoke([this, cable, sendtype](ZstTransportAdaptor * adaptor) {
		adaptor->send_message(ZstMsgKind::DESTROY_CABLE, sendtype, *cable, [this, cable](ZstMessageReceipt response) {
			this->destroy_cable_complete(response, cable);
		});
	});

	if (sendtype == ZstTransportSendType::SYNC_REPLY) process_events();
}

void ZstClientSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
	if (!cable) return;

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Destroy cable failed with status {}", ZstMsgNames[response.status]);
		return;
	}
	ZstLog::net(LogLevel::debug, "Destroy cable completed with status {}", ZstMsgNames[response.status]);
	
	//Remove the cable from our local cable list
	ZstSession::destroy_cable(cable);

	//Find the plugs and disconnect them seperately, in case they have already disappeared
	ZstInputPlug * input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(cable->get_input_URI()));
	ZstOutputPlug * output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(cable->get_output_URI()));

	//Remove cable from plugs
	if (input)
		plug_remove_cable(input, cable);

	if (output)
		plug_remove_cable(output, cable);

	cable->set_input(NULL);
	cable->set_output(NULL);

	//Queue events
	session_events().defer([cable](ZstSessionAdaptor * dlg) { dlg->on_cable_destroyed(cable); });
	synchronisable_enqueue_deactivation(cable);
}


// -----------------
// Event dispatchers
// -----------------

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientSession::stage_events()
{
	return m_stage_events;
}

ZstEventDispatcher<ZstTransportAdaptor*>& ZstClientSession::performance_events()
{
	return m_performance_events;
}

ZstClientHierarchy * ZstClientSession::hierarchy()
{
	return m_hierarchy;
}