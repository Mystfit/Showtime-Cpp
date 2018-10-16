#include "ZstClientSession.h"

ZstClientSession::ZstClientSession()
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

	//Attach this as an adaptor to the hierarchy module to handle hierarchy events
	m_hierarchy->hierarchy_events().add_adaptor(this);
}

void ZstClientSession::destroy()
{
	ZstSession::destroy();
	m_hierarchy->destroy();
}

void ZstClientSession::process_events()
{
	ZstSession::process_events();
	stage_events().process_events();
}

void ZstClientSession::flush()
{
	ZstSession::flush();
	stage_events().flush();
}

void ZstClientSession::dispatch_connected_to_stage()
{
	//Activate all owned entities
	synchronisable_enqueue_activation(static_cast<ZstClientHierarchy*>(hierarchy())->get_local_performer());
	session_events().defer([](ZstSessionAdaptor * adaptor) {
		adaptor->on_connected_to_stage();
	});
}

void ZstClientSession::dispatch_disconnected_from_stage()
{
	//Deactivate all owned entities
	synchronisable_enqueue_deactivation(static_cast<ZstClientHierarchy*>(hierarchy())->get_local_performer());
	session_events().defer([](ZstSessionAdaptor * adaptor) {
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
		const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>();
		create_cable(cable);
		break;
	}
	case ZstMsgKind::DESTROY_CABLE:
	{
		const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>();
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
	//Check if message has a payload
	if(msg->payload_size() < 1){
		ZstLog::net(LogLevel::warn, "No payload in graph message");
		return;
	}

	//Find local proxy for the sending plug
	ZstOutputPlug * sending_plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(ZstURI(msg->get_arg(ZstMsgArg::PATH))));
	ZstInputPlug * receiving_plug = NULL;

	if (!sending_plug) {
		ZstLog::net(LogLevel::warn, "No sending plug found");
		return;
	}
	
	//Create a ZstValue object to hold our plug data
	ZstValue received_val;
	received_val.read((char*)msg->payload_data(), msg->payload_size(), msg->payload_offset());

	//If the sending plug is a proxy then let the host app know it has updated
	if (sending_plug->is_proxy()) {
		ZstLog::net(LogLevel::debug, "Proxy plug {} received an update", sending_plug->URI().path());
		sending_plug->raw_value()->copy(received_val);
		synchronisable_annouce_update(sending_plug);
	}

	//Iterate over all connected cables from the sending plug
	ZstCableBundle bundle;
	for (auto cable : sending_plug->get_child_cables(bundle)) {
		receiving_plug = cable->get_input();
		if (receiving_plug) {
			if (!receiving_plug->is_proxy()) {
				receiving_plug->raw_value()->copy(received_val);
				plug_received_value(receiving_plug);
			}
		}
	}
}


void ZstClientSession::on_performer_leaving(ZstPerformer * performer)
{
	remove_connected_performer(performer);
}


void ZstClientSession::plug_received_value(ZstInputPlug * plug)
{
	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	if (!parent) {
		throw std::runtime_error("Could not find parent of input plug");
	}
	ZstURI plug_path = plug->URI();
	compute_events().defer([this, parent, plug_path](ZstComputeAdaptor * adaptor) {
		//Make sure the entity still exists before running 
		ZstInputPlug * plug = static_cast<ZstInputPlug*>(this->hierarchy()->find_entity(plug_path));
		if(plug)
			adaptor->on_compute(parent, plug);
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
		stage_events().invoke([this, sendtype, cable](ZstTransportAdaptor* adaptor) {
			adaptor->on_send_msg(ZstMsgKind::CREATE_CABLE, sendtype, *cable, [this, cable](ZstMessageReceipt response) {
				this->connect_cable_complete(response, cable);
			});
		});
	}

	if (sendtype == ZstTransportSendType::SYNC_REPLY) process_events();

	return cable;
}

void ZstClientSession::connect_cable_complete(ZstMessageReceipt response, ZstCable * cable) {
	if (response.status == ZstMsgKind::OK) {
		synchronisable_enqueue_activation(cable);
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), ZstMessage::get_msg_name(response.status));
	}
}

void ZstClientSession::destroy_cable(ZstCable * cable, const ZstTransportSendType & sendtype)
{
	ZstSession::destroy_cable(cable, sendtype);
	
	stage_events().invoke([this, cable, sendtype](ZstTransportAdaptor * adaptor) {
		adaptor->on_send_msg(ZstMsgKind::DESTROY_CABLE, sendtype, *cable, [this, cable](ZstMessageReceipt response) {
			this->destroy_cable_complete(response, cable);
		});
	});

	if (sendtype == ZstTransportSendType::SYNC_REPLY) process_events();
}

bool ZstClientSession::observe_entity(ZstEntityBase * entity, const ZstTransportSendType & sendtype)
{
	if (!ZstSession::observe_entity(entity, sendtype)) {
		return false;
	}

	stage_events().invoke([this, entity, sendtype](ZstTransportAdaptor * adaptor) {
		ZstMsgArgs args = { { ZstMsgArg::OUTPUT_PATH, entity->URI().first().path() } };
		adaptor->on_send_msg(ZstMsgKind::OBSERVE_ENTITY, sendtype, args, [this, entity](ZstMessageReceipt response) {
				this->observe_entity_complete(response, entity);
			}
		);
	});

	return true;
}


void ZstClientSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
	if (!cable) return;
	if (!cable->is_activated()) return;

	ZstSession::destroy_cable_complete(cable);

	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Destroy cable failed with status {}", ZstMessage::get_msg_name(response.status));
		return;
	}
	ZstLog::net(LogLevel::debug, "Destroy cable completed with status {}", ZstMessage::get_msg_name(response.status));

	//Queue events
	session_events().defer([cable](ZstSessionAdaptor * dlg) { dlg->on_cable_destroyed(cable); });
	synchronisable_enqueue_deactivation(cable);
}

void ZstClientSession::observe_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (response.status == ZstMsgKind::OK) {
		ZstLog::net(LogLevel::debug, "Observing entity {}", entity->URI().path());
		return;
	}
	ZstLog::net(LogLevel::error, "Observe entity {} failed with status {}", entity->URI().path(), ZstMessage::get_msg_name(response.status));
}


// -----------------
// Submodules
// -----------------

ZstClientHierarchy * ZstClientSession::hierarchy()
{
	return m_hierarchy;
}
