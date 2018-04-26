#include "ZstClientSession.h"

ZstClientSession::ZstClientSession()
{
	m_hierarchy = new ZstClientHierarchy();
}

ZstClientSession::~ZstClientSession() {
	delete m_hierarchy;
	delete m_reaper;
}


// ------------------------------
// Delegator overrides
// ------------------------------

void ZstClientSession::init(std::string client_name)
{
	m_hierarchy->init(client_name);
}

void ZstClientSession::destroy()
{
	m_hierarchy->destroy();
}

void ZstClientSession::process_events()
{
	ZstSession::process_events();
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::process_events();
	ZstEventDispatcher<ZstStageDispatchAdaptor*>::process_events();
	m_reaper->reap_all();
}

void ZstClientSession::flush()
{
	ZstSession::flush();
	ZstEventDispatcher<ZstSynchronisableAdaptor*>::flush();
	ZstEventDispatcher<ZstStageDispatchAdaptor*>::flush();
}

void ZstClientSession::on_connected_to_stage()
{
	m_hierarchy->synchronise_graph(true);
}

void ZstClientSession::on_disconnected_from_stage()
{
	//Deactivate all owned entities
	synchronisable_enqueue_deactivation(m_hierarchy->get_local_performer());
}


// ---------------------------
// Adaptor plug send/receive
// ---------------------------

void ZstClientSession::on_receive_from_performance(ZstMessage * msg)
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
			if (!receiving_plug->is_proxy()) {
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				this->plug_raw_value(receiving_plug)->read((char*)msg->payload_at(1).data(), msg->payload_at(1).size(), offset);
				add_event([receiving_plug](ZstSessionAdaptor * dlg) {dlg->on_plug_received_value(receiving_plug); });
			}
		}
	}
}

void ZstClientSession::on_plug_fire(ZstOutputPlug * plug)
{
	run_event([plug](ZstPerformanceDispatchAdaptor * adaptor) {
		adaptor->send_to_performance(plug);
	});
}

void ZstClientSession::on_plug_received_value(ZstInputPlug * plug)
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
		ZstLog::net(LogLevel::notification, "Didn't understand message type of {}", msg->payload_at(payload_index).kind());
		throw std::logic_error("Didn't understand message type");
		break;
	}
}

void ZstClientSession::on_synchronisable_destroyed(ZstSynchronisable * synchronisable)
{
	if(synchronisable->is_proxy())
		m_reaper->add(synchronisable);
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
		run_event([this, async, cable](ZstStageDispatchAdaptor* adaptor) {
			adaptor->send_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable, async, [this, cable](ZstMessageReceipt response) {
				this->connect_cable_complete(response, cable);
			});
		});
	}

	//Create the cable early so we have something to return immediately
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
	
	run_event([this, cable, async](ZstStageDispatchAdaptor * adaptor) {
		adaptor->send_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable, async, [this, cable](ZstMessageReceipt response) {
			this->destroy_cable_complete(response, cable);
		});
	});
}

void ZstClientSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
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


// -------------
// Cable queries
// -------------



ZstClientHierarchy * ZstClientSession::hierarchy()
{
	return m_hierarchy;
}


// -----------------------------
// Cable creation implementation
// -----------------------------
