#include "ZstClientSession.h"
#include "../core/ZstValue.h"
#include "../core/ZstPerformanceMessage.h"

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

void ZstClientSession::flush_events()
{
	ZstSession::flush_events();
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
	auto stage_msg = dynamic_cast<ZstStageMessage*>(msg);
	if (!stage_msg) {
		//Message was not from the stage - check if it is a performance message
		auto perf_msg = dynamic_cast<ZstPerformanceMessage*>(msg);
		if (perf_msg) {
			on_receive_graph_msg(perf_msg);
		}
		return;
	}

	switch (stage_msg->kind()) {
	case ZstMsgKind::CREATE_CABLE:
	{
		auto cable_address = stage_msg->unpack_payload_serialisable<ZstCableAddress>();
        auto input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(cable_address.get_input_URI()));
        auto output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(cable_address.get_output_URI()));
		auto cable = create_cable(input, output);
		if (!cable)
			throw std::runtime_error(fmt::format("Could not create cable from server request {}<-{}", cable_address.get_input_URI().path(), cable_address.get_output_URI().path()));

		ZstLog::net(LogLevel::debug, "Received cable from server {}<-{}", cable_address.get_input_URI().path(), cable_address.get_output_URI().path());
		break;
	}
	case ZstMsgKind::DESTROY_CABLE:
	{
		auto cable_address = stage_msg->unpack_payload_serialisable<ZstCableAddress>();
		ZstCable * cable_ptr = find_cable(cable_address);
		if (cable_ptr) {
			destroy_cable_complete(ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportSendType::SYNC_REPLY }, cable_ptr);
		}
		break;
	}
	default:
		break;
	}
}

void ZstClientSession::on_receive_graph_msg(ZstPerformanceMessage * msg)
{
	if(msg->payload().size() < 1 || msg->payload().is_null()){
        //"No payload in graph message"
		return;
	}
    
	//Find local proxy for the sending plug
	ZstOutputPlug * sending_plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(ZstURI(msg->sender().c_str(), msg->sender().size())));

	if (!sending_plug) {
		ZstLog::net(LogLevel::warn, "Received graph msg but could not find the sender plug. Sender: {}, Payload: {}, Payload size: {}", msg->sender().c_str(), msg->payload().dump().c_str(), msg->payload().dump().size());
		return;
	}
	
	//Create a ZstValue object to hold our plug data
	ZstValue received_val;
	received_val.read_json(msg->payload());

	//If the sending plug is a proxy then let the host app know it has updated
	if (sending_plug->is_proxy()) {
		sending_plug->raw_value()->copy(received_val);
		synchronisable_annouce_update(sending_plug);
	}

	//Iterate over all connected cables from the sending plug
	ZstCableBundle bundle;
	sending_plug->get_child_cables(bundle);
	for (auto cable : bundle) {
		auto receiving_plug = cable->get_input();
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
		stage_events().invoke([this, sendtype, cable](ZstTransportAdaptor* adaptor) {
			adaptor->send_msg(ZstMsgKind::CREATE_CABLE, sendtype, cable->get_address().as_json(), json::object(), [this, cable](ZstMessageReceipt response) {
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
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_address().get_input_URI().path(), cable->get_address().get_output_URI().path(), get_msg_name(response.status));
	}
}

void ZstClientSession::destroy_cable(ZstCable * cable, const ZstTransportSendType & sendtype)
{
	ZstSession::destroy_cable(cable, sendtype);
	
	stage_events().invoke([this, cable, sendtype](ZstTransportAdaptor * adaptor) {
		adaptor->send_msg(ZstMsgKind::DESTROY_CABLE, sendtype, cable->get_address().as_json(), json(), [this, cable](ZstMessageReceipt response) {
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
		ZstMsgArgs args = { 
			{ get_msg_arg_name(ZstMsgArg::OUTPUT_PATH), entity->URI().first().path() } 
		};
		adaptor->send_msg(ZstMsgKind::OBSERVE_ENTITY, sendtype, args, [this, entity](ZstMessageReceipt response) {
				this->observe_entity_complete(response, entity);
			}
		);
	});

	return true;
}


void ZstClientSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
	ZstSession::destroy_cable_complete(cable);
	if (response.status != ZstMsgKind::OK) {
		ZstLog::net(LogLevel::error, "Destroy cable failed with status {}", get_msg_name(response.status));
		return;
	}
	ZstLog::net(LogLevel::debug, "Destroy cable completed with status {}", get_msg_name(response.status));
}

void ZstClientSession::observe_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
	if (response.status == ZstMsgKind::OK) {
		ZstLog::net(LogLevel::debug, "Observing entity {}", entity->URI().path());
		return;
	}
	ZstLog::net(LogLevel::error, "Observe entity {} failed with status {}", entity->URI().path(), get_msg_name(response.status));
}


// -----------------
// Submodules
// -----------------

ZstClientHierarchy * ZstClientSession::hierarchy()
{
	return m_hierarchy;
}
