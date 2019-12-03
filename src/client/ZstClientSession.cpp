#include "ZstClientSession.h"
#include "../core/ZstValue.h"
#include "../core/ZstPerformanceMessage.h"

using namespace flatbuffers;

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

void ZstClientSession::on_receive_msg(std::shared_ptr<ZstStageMessage> msg)
{
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
    
void ZstClientSession::on_receive_msg(std::shared_ptr<ZstPerformanceMessage> msg)
{
    //Find local proxy for the sending plug
	
	auto sender = ZstURI(msg->buffer()->sender()->c_str(), msg->buffer()->sender()->size());
    ZstOutputPlug * sending_plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(sender));
    
    if (!sending_plug) {
        ZstLog::net(LogLevel::warn, "Received graph msg but could not find the sender plug. Sender: {}", sender.path());
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
    
	for (auto cable : *request->cables()) {
		auto address = ZstCableAddress(cable);
		auto input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(address.get_input_URI()));
		auto output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(address.get_output_URI()));
		auto cable_proxy = create_cable(input, output);

		if (cable_proxy)
			ZstLog::net(LogLevel::debug, "Received cable from server {}", cable_proxy->get_address().to_string());
	}
}
    
void ZstClientSession::cable_destroy_handler(const CableDestroyRequest* request)
{
	for (auto address : *request->cables()) {
		ZstCable* cable_ptr = find_cable(ZstCableAddress(address));
		if (cable_ptr)
			destroy_cable_complete(ZstMessageReceipt{ Signal_OK }, cable_ptr);
	}
}

void ZstClientSession::aquire_entity_ownership_handler(const EntityTakeOwnershipRequest* request)
{
    auto entity = hierarchy()->find_entity(ZstURI(request->URI()->c_str(), request->URI()->size()));
    
    // Set the owner
    ZstEntityBundle bundle;
    entity->get_child_entities(bundle);
    for(auto child : bundle){
        entity_set_owner(child, ZstURI(request->new_owner()->c_str(), request->new_owner()->size()));
    }
}

void ZstClientSession::on_performer_leaving(ZstPerformer * performer)
{
	remove_connected_performer(performer->URI());
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
				if (response.status == Signal_OK) {
					this->connect_cable_complete(response, cable);
				} else {
					ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_address().get_input_URI().path(), cable->get_address().get_output_URI().path(), EnumNameSignal(response.status));
				}
			};
            
			auto builder = FlatBufferBuilder();
			auto cable_vec = std::vector<Offset<Cable> >{ 
				CreateCable(builder, CreateCableData(builder, builder.CreateString(cable->get_input()->URI().path()), builder.CreateString(cable->get_output()->URI().path())))
			};
            auto cable_msg = CreateCableCreateRequest(builder, builder.CreateVector(cable_vec));
            adaptor->send_msg(Content_CableCreateRequest, cable_msg.Union(), builder, args);
		});
	}

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) process_events();

	return cable;
}

void ZstClientSession::connect_cable_complete(ZstMessageReceipt response, ZstCable * cable) {
	
	synchronisable_enqueue_activation(cable);
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
        auto address_offset =  CreateCableData(builder, builder.CreateString(cable->get_input()->URI().path()), builder.CreateString(cable->get_output()->URI().path()));
		std::vector<Offset<Cable > > cable_vec{ CreateCable(builder, address_offset) };
		auto destroy_cable_msg = CreateCableDestroyRequest(builder, builder.CreateVector(cable_vec));
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
    stage_events()->invoke([entity, this](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
        args.on_recv_response = [](ZstMessageReceipt) {
            ZstLog::net(LogLevel::debug, "Ack from server");
        };
        
        FlatBufferBuilder builder;
        auto entity_own_msg = CreateEntityTakeOwnershipRequest(builder, builder.CreateString(entity->URI().path()), builder.CreateString(hierarchy()->get_local_performer()->URI().path()));
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
        
		// Sending an empty string for the owner will release entity ownership back to the original owner
		FlatBufferBuilder builder;
        auto release_ownership_msg = CreateEntityTakeOwnershipRequest(builder, builder.CreateString(entity->URI().path()), builder.CreateString(""));
        adaptor->send_msg(Content_EntityTakeOwnershipRequest, release_ownership_msg.Union(), builder, args);
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
