#include "ZstClientSession.h"
#include "../core/transports/ZstStageTransport.h"
#include "../core/ZstPerformanceMessage.h"
#include <format>
#include <showtime/entities/ZstComputeComponent.h>

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
    init_adaptors();
	m_hierarchy->init(client_name);
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

void ZstClientSession::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg)
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
    
void ZstClientSession::on_receive_msg(const std::shared_ptr<ZstPerformanceMessage>& msg)
{
    if (msg->buffer()->value()->values_type() == PlugValueData_PlugHandshake) {
        return;
    }

    //Find local proxy for the sending plug
	auto sender = ZstURI(msg->buffer()->sender()->c_str(), msg->buffer()->sender()->size());
    ZstOutputPlug * sending_plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(sender));
    
    if (!sending_plug) {
        Log::net(Log::Level::warn, "Received graph msg but could not find the sender plug. Sender: {}", sender.path());
        return;
    }
    
    //If the sending plug is a proxy then let the host app know it has updated
    if (sending_plug->is_proxy()) {
        sending_plug->raw_value()->deserialize(msg->buffer()->value());
        synchronisable_annouce_update(sending_plug);
    }
    
    //Iterate over all connected cables from the sending plug
    ZstCableBundle bundle;
    sending_plug->get_child_cables(&bundle);
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
    
	auto address = ZstCableAddress(request->cable());
	auto input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(address.get_input_URI()));
	auto output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(address.get_output_URI()));
	auto cable_proxy = create_cable(input, output);

	if (cable_proxy)
		Log::net(Log::Level::debug, "Received cable from server {}", cable_proxy->get_address().to_string());
}
    
void ZstClientSession::cable_destroy_handler(const CableDestroyRequest* request)
{
	ZstCable* cable_ptr = find_cable(ZstCableAddress(request->cable()));
    ZstSession::destroy_cable_complete(cable_ptr);
}

void ZstClientSession::aquire_entity_ownership_handler(const EntityTakeOwnershipRequest* request)
{
    auto entity = hierarchy()->find_entity(ZstURI(request->URI()->c_str(), request->URI()->size()));
	if (!entity)
		return;

    // Set the owner
    ZstEntityBundle bundle;
    entity->get_child_entities(&bundle, true, true);
    for(auto child : bundle){
        entity_set_owner(child, ZstURI(request->new_owner()->c_str(), request->new_owner()->size()));
    }
}

void ZstClientSession::on_performer_leaving(const ZstURI& performer_path)
{
	remove_connected_performer(performer_path);
}

void ZstClientSession::plug_received_value(ZstInputPlug * plug)
{
    auto parent_ent = plug->parent();
	if (!parent_ent) {
		throw std::runtime_error("Could not find parent of input plug");
	}
    
    if (plug->triggers_compute()) {
        ZstURI plug_path = plug->URI();
        compute_events()->defer([this, parent_path = parent_ent->URI(), plug_path](ZstComputeAdaptor* adaptor) {
            //Make sure the entity still exists before running 
            if (auto entity = this->hierarchy()->find_entity(plug_path)) {
                if (entity->entity_type() == ZstEntityType::PLUG) {
                    auto plug = static_cast<ZstPlug*>(entity);
                    if (plug->direction() == ZstPlugDirection::IN_JACK) {

                        if (auto compute_comp = dynamic_cast<ZstComputeComponent*>(this->hierarchy()->find_entity(parent_path))) {
                            adaptor->on_request_compute(compute_comp, static_cast<ZstInputPlug*>(plug));
                        }
                    }
                }
            }
        });
    }
}



// ------------------
// Cable creation API
// ------------------

ZstCable * ZstClientSession::connect_cable(ZstInputPlug * input, ZstOutputPlug * output, const ZstTransportRequestBehaviour & sendtype)
{
	ZstCable * cable = NULL;
	cable = ZstSession::connect_cable(input, output, sendtype);
	
	if (cable) {
		stage_events()->invoke([this, sendtype, cable](ZstStageTransportAdaptor* adaptor) {
			ZstTransportArgs args;
			args.msg_send_behaviour = sendtype;
			args.on_recv_response = [this, cable](ZstMessageResponse response) { 
                if (ZstStageTransport::verify_signal(response.response, Signal_OK, 
                    std::format("Cable connect ({}-->{})", 
                                cable->get_address().get_output_URI().path(), 
                                cable->get_address().get_input_URI().path())
                )) {
                    this->connect_cable_complete(response, cable);
                }
			};
            
            auto builder = std::make_shared< FlatBufferBuilder>();
            auto cable_msg = CreateCableCreateRequest(*builder,
                CreateCable(*builder,
                    CreateCableData(*builder,
                        builder->CreateString(cable->get_address().get_input_URI().path()), 
                        builder->CreateString(cable->get_address().get_output_URI().path())
                    )));
            adaptor->send_msg(Content_CableCreateRequest, cable_msg.Union(), builder, args);
		});
	}

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) process_events();

	return cable;
}

void ZstClientSession::connect_cable_complete(ZstMessageResponse response, ZstCable * cable) {
	
	synchronisable_enqueue_activation(cable);
}

void ZstClientSession::destroy_cable(ZstCable * cable, const ZstTransportRequestBehaviour & sendtype)
{
	if (!cable)
		return;

	ZstSession::destroy_cable(cable, sendtype);
	
	stage_events()->invoke([this, cable, sendtype](ZstStageTransportAdaptor* adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, cable](ZstMessageResponse response) { this->destroy_cable_complete(response, cable); };
        
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto address_offset =  CreateCableData(*builder,
            builder->CreateString(cable->get_input()->URI().path()),
            builder->CreateString(cable->get_output()->URI().path())
        );
		auto destroy_cable_msg = CreateCableDestroyRequest(*builder, CreateCable(*builder, address_offset));
        adaptor->send_msg(Content_CableDestroyRequest, destroy_cable_msg.Union(), builder, args);
	});

	if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) process_events();
}

void ZstClientSession::destroy_cable_complete(ZstMessageResponse response, ZstCable * cable)
{
    if (!ZstStageTransport::verify_signal(response.response, Signal_OK, "Destroy cable"))
        return;
    ZstSession::destroy_cable_complete(cable);
}

bool ZstClientSession::observe_entity(ZstEntityBase * entity, const ZstTransportRequestBehaviour & sendtype)
{
	if (!ZstSession::observe_entity(entity, sendtype)) {
		return false;
	}

	stage_events()->invoke([this, entity, sendtype](ZstStageTransportAdaptor* adaptor) {
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, entity](ZstMessageResponse response) { this->observe_entity_complete(response, entity); };
        
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto observe_msg = CreateEntityObserveRequest(*builder, builder->CreateString(entity->URI().path()));
		adaptor->send_msg(Content_EntityObserveRequest, observe_msg.Union(), builder, args);
	});

	return true;
}
    
void ZstClientSession::observe_entity_complete(ZstMessageResponse response, ZstEntityBase * entity)
{
    if (ZstStageTransport::verify_signal(response.response, Signal_OK, std::format("Observer entity {}", entity->URI().path())))
        Log::net(Log::Level::debug, "Observing entity {} completed successfully", entity->URI().path());
}

void ZstClientSession::aquire_entity_ownership(ZstEntityBase* entity)
{
    stage_events()->invoke([entity, this](ZstStageTransportAdaptor* adaptor) {
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
        args.on_recv_response = [](ZstMessageResponse response) {
            Log::net(Log::Level::debug, "Ack from server");
        };
        
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto entity_own_msg = CreateEntityTakeOwnershipRequest(*builder, builder->CreateString(entity->URI().path()), builder->CreateString(hierarchy()->get_local_performer()->URI().path()));
        adaptor->send_msg(Content_EntityTakeOwnershipRequest, entity_own_msg.Union(), builder, args);
    });
}

void ZstClientSession::release_entity_ownership(ZstEntityBase* entity)
{
    stage_events()->invoke([entity](ZstStageTransportAdaptor* adaptor) {
        ZstTransportArgs args;
        args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
        args.on_recv_response = [](ZstMessageResponse){
            Log::net(Log::Level::debug, "Ack from server");
        };
        
		// Sending an empty string for the owner will release entity ownership back to the original owner
        auto builder = std::make_shared< FlatBufferBuilder>();
        auto release_ownership_msg = CreateEntityTakeOwnershipRequest(*builder, builder->CreateString(entity->URI().path()), builder->CreateString(""));
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
