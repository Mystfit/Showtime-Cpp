#include "ZstClientSession.h"
#include "../core/ZstValue.h"
#include "../core/ZstPerformanceMessage.h"

namespace showtime::client {

ZstClientSession::ZstClientSession() : m_hierarchy(std::make_shared<ZstClientHierarchy>())
{
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
}

void ZstClientSession::process_events()
{
	ZstSession::process_events();
	ZstClientModule::process_events();
}

void ZstClientSession::flush_events()
{
	ZstSession::flush_events();
	ZstClientModule::flush_events();
}

void ZstClientSession::dispatch_connected_to_stage()
{
	//Activate all owned entities
	synchronisable_enqueue_activation(hierarchy()->get_local_performer());
}

void ZstClientSession::dispatch_disconnected_from_stage()
{
	//Deactivate all owned entities
	synchronisable_enqueue_deactivation(hierarchy()->get_local_performer());
}


// ---------------------------
// Adaptor plug send/receive
// ---------------------------

void ZstClientSession::on_receive_msg(const ZstStageMessage * msg)
{
//    switch (stage_msg->kind()) {
//    case ZstMsgKind::CREATE_CABLE:
//    {
//        auto cable_address = stage_msg->unpack_payload_serialisable<ZstCableAddress>();
//        auto input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(cable_address.get_input_URI()));
//        auto output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(cable_address.get_output_URI()));
//        auto cable = create_cable(input, output);
//        if (!cable)
//            throw std::runtime_error(fmt::format("Could not create cable from server request {}<-{}", cable_address.get_input_URI().path(), cable_address.get_output_URI().path()));
//
//        ZstLog::net(LogLevel::debug, "Received cable from server {}<-{}", cable_address.get_input_URI().path(), cable_address.get_output_URI().path());
//        break;
//    }
//    case ZstMsgKind::DESTROY_CABLE:
//    {
//        auto cable_address = stage_msg->unpack_payload_serialisable<ZstCableAddress>();
//        ZstCable * cable_ptr = find_cable(cable_address);
//        if (cable_ptr) {
//            destroy_cable_complete(ZstMessageReceipt{ ZstMsgKind::OK }, cable_ptr);
//        }
//        break;
//    }
//    case ZstMsgKind::AQUIRE_ENTITY_OWNERSHIP:
//        aquire_entity_ownership_handler(stage_msg);
//        break;
//    default:
//        break;
//    }
    switch (msg->type()) {
        case Content_CableCreateRequest:
            cable_create_handler(msg->buffer()->content_as_CableCreateRequest());
            break;
        case Content_CableDestroyRequest:
            cable_destroy_handler(msg->buffer()->content_as_CableDestroyRequest());
            break;
        case Content_EntityTakeOwnershipRequest:
            aquire_entity_ownership_handler(msg->buffer()->content_as_EntityTakeOwnershipRequest());
            break;
        default:
            break;
    }
}
    
void ZstClientSession::on_receive_msg(const ZstPerformanceMessage * msg)
{
    //Find local proxy for the sending plug
    ZstOutputPlug * sending_plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(ZstURI(msg->sender().c_str(), msg->sender().size())));
    
    if (!sending_plug) {
        ZstLog::net(LogLevel::warn, "Received graph msg but could not find the sender plug. Sender: {}, Payload: {}, Payload size: {}", msg->sender().c_str());
        return;
    }
    
    //If the sending plug is a proxy then let the host app know it has updated
    if (sending_plug->is_proxy()) {
        sending_plug->raw_value()->deserialize(msg->buffer()->value());
        synchronisable_annouce_update(sending_plug);
    }
    
    //Iterate over all connected cables from the sending plug
    ZstCableBundle bundle;
    sending_plug->get_child_cables(bundle);
    for (auto cable : bundle) {
        auto receiving_plug = cable->get_input();
        if (receiving_plug) {
            if (!receiving_plug->is_proxy()) {
                receiving_plug->raw_value()->deserialize(msg->buffer()->value());
                plug_received_value(receiving_plug);
            }
        }
    }
}
    
void ZstClientSession::cable_create_handler(const CableCreateRequest* request){
    
    auto address = ZstCableAddress(request->address());
    auto input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(address.get_input_URI()));
    auto output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(address.get_output_URI()));
    auto cable = create_cable(input, output);
    
    if(cable)
        ZstLog::net(LogLevel::debug, "Received cable from server {}", cable->get_address().to_string());
}
    
void ZstClientSession::cable_destroy_handler(const CableDestroyRequest* request)
{
    ZstCable * cable_ptr = find_cable(ZstCableAddress(request->address()));
    if (cable_ptr)
        destroy_cable_complete(ZstMessageReceipt{ Signal_OK }, cable_ptr);
}

void ZstClientSession::aquire_entity_ownership_handler(const EntityTakeOwnershipRequest* request)
{
    
    auto entity_path = stage_msg->get_arg<std::string>(ZstMsgArg::PATH);
    auto owner_path = stage_msg->get_arg<std::string>(ZstMsgArg::OUTPUT_PATH);
    auto entity = hierarchy()->find_entity(ZstURI(entity_path.c_str(), entity_path.size()));
    auto owner = ZstURI(owner_path.c_str(), owner_path.size());
    
    // Set the owner
    ZstEntityBundle bundle;
    entity->get_child_entities(bundle);
    for(auto child : bundle){
        entity_set_owner(child, owner);
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
	compute_events()->defer([this, parent, plug_path](std::shared_ptr<ZstComputeAdaptor> adaptor) {
		//Make sure the entity still exists before running 
		ZstInputPlug * plug = static_cast<ZstInputPlug*>(this->hierarchy()->find_entity(plug_path));
		if(plug)
			adaptor->on_compute(parent, plug);
	});
}



// ------------------
// Cable creation API
// ------------------

ZstCable * ZstClientSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportRequestBehaviour & sendtype)
{
	ZstCable * cable = NULL;
	cable = ZstSession::connect_cable(input, output, sendtype);
	
	if (cable) {
		stage_events()->invoke([this, sendtype, cable](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
			ZstTransportArgs args;
			args.msg_send_behaviour = sendtype;
			args.on_recv_response = [this, cable](ZstMessageReceipt response) { 
				this->connect_cable_complete(response, cable); 
			};
            
            
            auto builder = FlatBufferBuilder();
            auto cable_msg = CreateCable(builder, builder.CreateString(cable->get_input()->URI().path()), builder.CreateString(cable->get_output()->URI().path()));
            adaptor->send_msg(Content_CableCreateRequest, cable_msg.Union(), builder, args);
		});
	}

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) process_events();

	return cable;
}

void ZstClientSession::connect_cable_complete(ZstMessageReceipt response, ZstCable * cable) {
	if (response.status == Signal_OK) {
		synchronisable_enqueue_activation(cable);
	}
	else {
		ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_address().get_input_URI().path(), cable->get_address().get_output_URI().path(), EnumNameSignal(response.status));
	}
}

void ZstClientSession::destroy_cable(ZstCable * cable, const ZstTransportRequestBehaviour & sendtype)
{
	if (!cable)
		return;

	ZstSession::destroy_cable(cable, sendtype);
	
	stage_events()->invoke([this, cable, sendtype](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, cable](ZstMessageReceipt response) { this->destroy_cable_complete(response, cable); };
        
        FlatBufferBuilder builder;
        auto cable_offset =  CreateCable(builder, builder.CreateString(cable->get_input()->URI().path()), builder.CreateString(cable->get_output()->URI().path()));
        auto destroy_cable_msg = CreateCableDestroyRequest(builder, cable_offset);
        adaptor->send_msg(Content_CableDestroyRequest, destroy_cable_msg.Union(), builder, args);
	});

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) process_events();
}

void ZstClientSession::destroy_cable_complete(ZstMessageReceipt response, ZstCable * cable)
{
    ZstSession::destroy_cable_complete(cable);
    if (response.status != Signal_OK) {
        ZstLog::net(LogLevel::error, "Destroy cable failed with status {}", EnumNameSignal(response.status));
        return;
    }
    ZstLog::net(LogLevel::debug, "Destroy cable completed with status {}", EnumNameSignal(response.status));
}

bool ZstClientSession::observe_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!ZstSession::observe_entity(entity, sendtype)) {
		return false;
	}

	stage_events()->invoke([this, entity, sendtype](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, entity](ZstMessageReceipt response) { this->observe_entity_complete(response, entity); };
        
        FlatBufferBuilder builder;
        auto observe_msg = CreateEntityObserveRequest(builder, builder.CreateString(entity->URI().path()));
		adaptor->send_msg(Content_EntityObserveRequest, observe_msg.Union(), builder, args);
	});

	return true;
}
    
void ZstClientSession::observe_entity_complete(ZstMessageReceipt response, ZstEntityBase * entity)
{
    if (response.status == Signal_OK) {
        ZstLog::net(LogLevel::debug, "Observing entity {}", entity->URI().path());
        return;
    }
    ZstLog::net(LogLevel::error, "Observe entity {} failed with status {}", entity->URI().path(), EnumNameSignal(response.status));
}

void ZstClientSession::aquire_entity_ownership(ZstEntityBase* entity)
{
    stage_events()->invoke([entity](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
        args.on_recv_response = [](ZstMessageReceipt) {
            ZstLog::net(LogLevel::debug, "Ack from server");
        };
        
        FlatBufferBuilder builder;
        auto entity_own_msg = CreateEntityTakeOwnershipRequest(builder, builder.CreateString(entity->URI().path()));
        adaptor->send_msg(Content_EntityTakeOwnershipRequest, entity_own_msg.Union(), builder, args);
    });
}

void ZstClientSession::release_entity_ownership(ZstEntityBase* entity)
{
    stage_events()->invoke([entity](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
        args.on_recv_response = [](ZstMessageReceipt){
            ZstLog::net(LogLevel::debug, "Ack from server");
        };
        
        FlatBufferBuilder builder;
        auto release_ownership_msg = CreateEntityReleaseOwnershipRequest(builder, builder.CreateString(entity->URI().path()));
        adaptor->send_msg(Content_EntityReleaseOwnershipRequest, release_ownership_msg.Union(), builder, args);
    });
}
    
// -----------------
// Submodules
// -----------------

std::shared_ptr<ZstHierarchy> ZstClientSession::hierarchy()
{
	return m_hierarchy;
}

}
