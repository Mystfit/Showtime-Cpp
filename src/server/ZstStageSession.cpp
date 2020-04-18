#include "ZstStageSession.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace boost::uuids;
using namespace flatbuffers;

namespace showtime {

ZstStageSession::ZstStageSession() : m_hierarchy(std::make_shared<ZstStageHierarchy>())
{
}

ZstStageSession::~ZstStageSession()
{
	//ZstCableBundle bundle;
	//for (auto cable : get_cables(bundle)) {
	//	destroy_cable(cable);
	//}
}

void ZstStageSession::process_events()
{
	ZstSession::process_events();
	ZstStageModule::process_events();
	hierarchy()->process_events();
}

void ZstStageSession::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
	ZstStageModule::set_wake_condition(condition);
	m_hierarchy->set_wake_condition(condition);
	session_events()->set_wake_condition(condition);
	synchronisable_events()->set_wake_condition(condition);
}

void ZstStageSession::on_receive_msg(std::shared_ptr<ZstStageMessage> msg)
{
	Signal response = Signal_EMPTY;
	ZstPerformerStageProxy* sender = m_hierarchy->get_client_from_endpoint_UUID(msg->endpoint_UUID());

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (msg->type()) {
	case Content_SignalMessage:
		response = signal_handler(msg->buffer()->content_as_SignalMessage(), sender);
		break;
	case Content_CableCreateRequest:
		response = create_cable_handler(msg->buffer(), sender);
		break;
	case Content_CableDestroyRequest:
		response = destroy_cable_handler(msg->buffer()->content_as_CableDestroyRequest());
		break;
	case Content_EntityObserveRequest:
		response = observe_entity_handler(msg->buffer(), sender);
		break;
	case Content_EntityTakeOwnershipRequest:
		response = aquire_entity_ownership_handler(msg->buffer()->content_as_EntityTakeOwnershipRequest(), sender);
		break;
	default:
		break;
	}

	//Return response to sender
	if (response != Signal_EMPTY) {
		ZstTransportArgs args;
		args.target_endpoint_UUID = msg->endpoint_UUID();
		args.msg_ID = msg->id();

		router_events()->defer([response, args](std::shared_ptr<ZstStageTransportAdaptor> adaptor) {
			FlatBufferBuilder builder;
			auto signal_offset = CreateSignalMessage(builder, response);
			adaptor->send_msg(Content_SignalMessage, signal_offset.Union(), builder, args);
		});
	}
}

Signal ZstStageSession::signal_handler(const SignalMessage* request, ZstPerformerStageProxy* sender)
{
	if (!sender) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	switch (request->signal()) {
	case Signal_CLIENT_SYNC:
		return synchronise_client_graph_handler(sender);
		break;
	}

	return Signal_OK;
}

Signal ZstStageSession::synchronise_client_graph_handler(ZstPerformerStageProxy* sender) 
{
	ZstLog::server(LogLevel::notification, "Sending graph snapshot to {}", sender->URI().path());

	// For serialisation later
	auto builder = std::make_shared<FlatBufferBuilder>();
	std::vector< flatbuffers::Offset<void> > entity_vec;
	std::vector< uint8_t> entity_types_vec;

	ZstEntityBundle entity_bundle;
	ZstEntityBundle performer_bundle;
	hierarchy()->get_performers(performer_bundle);

	// Pack all entities
	for (auto performer : performer_bundle) {
		//Only pack performers that aren't the destination client
		if (performer->URI() != sender->URI()) {
			performer->get_child_entities(entity_bundle, true, true);
		}
	}

	if (entity_bundle.size()) {
		for (auto entity : entity_bundle) {
			auto batch_entity_offset = CreateEntityCreateRequest(*builder, entity->serialized_entity_type(), entity->serialize(*builder));
			stage_hierarchy()->whisper(sender, Content_EntityCreateRequest, batch_entity_offset.Union(), builder, ZstTransportArgs());
		}
	}

	// Pack all cables
	// Create a new buffer builder
	if (m_cables.size()) {
		builder = std::make_shared<FlatBufferBuilder>();
		for (auto const& cable : m_cables) {
			auto batch_cable_offset = CreateCableCreateRequest(*builder, cable->get_address().serialize(*builder));
			stage_hierarchy()->whisper(sender, Content_CableCreateRequest, batch_cable_offset.Union(), builder, ZstTransportArgs());
		}
		
	}
	return Signal_OK;
}

Signal ZstStageSession::create_cable_handler(const StageMessage* request, ZstPerformerStageProxy* sender)
{
	auto cable_request = request->content_as_CableCreateRequest();
    ZstMsgID id;
    memcpy(&id, request->id()->data(), request->id()->size());

	//Unpack cable from message
	auto input_path = ZstURI(cable_request->cable()->address()->input_URI()->c_str(), cable_request->cable()->address()->input_URI()->size());
	auto output_path = ZstURI(cable_request->cable()->address()->output_URI()->c_str(), cable_request->cable()->address()->output_URI()->size());
	auto cable_path = ZstCableAddress(input_path, output_path);
	ZstLog::server(LogLevel::notification, "Received connect cable request for In:{} and Out:{}", cable_path.get_input_URI().path(), cable_path.get_output_URI().path());

	//Make sure cable doesn't already exist
	if (find_cable(cable_path)) {
		ZstLog::server(LogLevel::warn, "Cable {}<-{} already exists", input_path.path(), output_path.path());
		return Signal_ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Verify plugs will accept cable
	ZstInputPlug* input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(input_path));
	ZstOutputPlug* output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(output_path));
	if (!input || !output) {
		return Signal_ERR_CABLE_PLUGS_NOT_FOUND;
	}

	//Unplug existing cables if we hit max cables in an input plug
	if (input->num_cables() >= input->max_connected_cables()) {
		ZstLog::server(LogLevel::warn, "Too many cables in plug. Disconnecting existing cables");
		ZstCableBundle bundle;
		input->get_child_cables(bundle);
		for (auto cable : bundle) {
			destroy_cable(cable);
		}
	}

	//Create our local cable ptr
	auto cable_ptr = create_cable(input, output);
	if (!cable_ptr) {
		return Signal_ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Start the client connection
	auto input_perf = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(input->URI().first()));
	auto output_perf = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(output->URI().first()));
	connect_clients(output_perf, input_perf, [this, cable_ptr, sender, id](ZstMessageReceipt receipt) {
		if (receipt.status == Signal_OK) {
			ZstLog::server(LogLevel::notification, "Client connection complete. Publishing cable {}", cable_ptr->get_address().to_string());
			ZstTransportArgs args;
			auto builder = std::make_shared<FlatBufferBuilder>();

			// Publish cable
			auto cable_create_offset = CreateCableCreateRequest(*builder, cable_ptr->get_address().serialize(*builder));
			stage_hierarchy()->broadcast(Content_CableCreateRequest, cable_create_offset.Union(), builder, args);
		}

		// Let original requestor know the request was completed
		stage_hierarchy()->reply_with_signal(sender, receipt.status, id);
	});

	

	return Signal_EMPTY;
}


//---------------------

Signal ZstStageSession::observe_entity_handler(const StageMessage* request, ZstPerformerStageProxy* sender)
{
	//Get target performer
	auto observe_request = request->content_as_EntityObserveRequest();
	auto performer_path = ZstURI(observe_request->URI()->c_str(), observe_request->URI()->size());
	ZstPerformerStageProxy* observed_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(performer_path.first()));
	if (!observed_performer) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	ZstLog::server(LogLevel::notification, "Received observation request. Requestor: {}, Observed: {}", sender->URI().path(), observed_performer->URI().path());
	if (sender == observed_performer) {
		ZstLog::server(LogLevel::warn, "Client attempting to observe itself");
		return Signal_ERR_STAGE_PERFORMER_ALREADY_CONNECTED;
	}

	//Check to see if one client is already connected to the other
	if (observed_performer->has_connected_subscriber(sender)) {
		ZstLog::server(LogLevel::warn, "Client {} already observing {}", sender->URI().path(), observed_performer->URI().path());
		return Signal_ERR_STAGE_PERFORMER_ALREADY_CONNECTED;
	}

	//Start the client connection
    ZstMsgID response_id;
    memcpy(&response_id, request->id()->data(), request->id()->size());
    
	connect_clients(observed_performer, sender, [this, sender, response_id](ZstMessageReceipt receipt) {
		stage_hierarchy()->reply_with_signal(sender, Signal_OK, response_id);
	});

	return Signal_EMPTY;
}


Signal ZstStageSession::aquire_entity_ownership_handler(const EntityTakeOwnershipRequest* request, ZstPerformerStageProxy* sender)
{
	auto entity_path = ZstURI(request->URI()->c_str(), request->URI()->size());
	ZstEntityBase* entity = hierarchy()->find_entity(entity_path);

	if (!entity) {
		ZstLog::server(LogLevel::warn, "Could not aquire entity ownership - entity {}, not found", entity_path.path());
		return Signal_ERR_ENTITY_NOT_FOUND;
	}

	ZstURI new_owner_path;
	ZstPerformerStageProxy* new_owner = NULL;

	if (!request->new_owner()->size()){
		//Reset owner
		entity_set_owner(entity, "");
	}
	else {
		new_owner_path = ZstURI(request->new_owner()->c_str(), request->new_owner()->size());
		new_owner = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(new_owner_path));
		if (!new_owner) {
			ZstLog::server(LogLevel::warn, "Could not aquire entity ownership - could not find new owner {}", new_owner_path.path());
			return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
		}

		ZstLog::server(LogLevel::notification, "Received entity ownership aquistion request - {} wants to control {}", new_owner_path.path(), entity->URI().path());

		//Set owner
		entity_set_owner(entity, new_owner->URI());
	}

	// We need to connect downstream plugs to the new sender
	ZstCableBundle bundle;
	get_cables(bundle);

	//Find all performers that have input connections to this output plug
	std::unordered_map<ZstURI, ZstPerformerStageProxy*, ZstURIHash> performers;
	for (auto c : bundle) {
		if (c->get_output()->URI() == entity->URI()) {
			auto receiver = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(c->get_input()->URI().first()));
			performers[receiver->URI()] = receiver;
		}
	}

	//Connect performers together that will have to update their subscriptions
	for (auto receiver : performers) {
		if (!new_owner_path.is_empty()) {
			connect_clients(receiver.second, new_owner);
		}
	}

	//Broadcast change in plug fire control
	ZstLog::server(LogLevel::notification, "Broadcasting entity ownership - {} controls {}", new_owner_path.path(), entity->URI().path());
	ZstTransportArgs args;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto ownership_offset = CreateEntityTakeOwnershipRequest(*builder, builder->CreateString(entity->URI().path()), builder->CreateString(new_owner_path.path()));
	stage_hierarchy()->broadcast(Content_EntityTakeOwnershipRequest, ownership_offset.Union(), builder, args);

	return Signal_OK;
}

Signal ZstStageSession::destroy_cable_handler(const CableDestroyRequest* request)
{
	auto cable_path = ZstCableAddress(request->cable());
	ZstLog::server(LogLevel::notification, "Received destroy cable connection request");

	ZstCable* cable_ptr = find_cable(cable_path);
	destroy_cable(cable_ptr);
	
	return Signal_OK;
}

void ZstStageSession::on_performer_leaving(ZstPerformer* performer)
{
	disconnect_cables(performer);
}

void ZstStageSession::on_entity_leaving(ZstEntityBase* entity)
{
	disconnect_cables(entity);
}

void ZstStageSession::on_plug_leaving(ZstPlug* plug)
{
	disconnect_cables(plug);
}

void ZstStageSession::disconnect_cables(ZstEntityBase* entity)
{
	ZstCableBundle bundle;
 	entity->get_child_cables(bundle);
	for (auto cable : bundle) {
		destroy_cable(cable);
	}
}

void ZstStageSession::destroy_cable(ZstCable* cable) {
	if (!cable)
		return;

	ZstLog::server(LogLevel::notification, "Destroying cable {}", cable->get_address().to_string());

	//Update rest of network
	ZstTransportArgs args;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto destroy_cable_data_offset = CreateCableData(*builder, builder->CreateString(cable->get_input()->URI().path()), builder->CreateString(cable->get_output()->URI().path()));
	auto destroy_cable_offset = CreateCableDestroyRequest(*builder, CreateCable(*builder, destroy_cable_data_offset));
	stage_hierarchy()->broadcast(Content_CableDestroyRequest, destroy_cable_offset.Union(), builder, args);

	//Remove cable
	ZstSession::destroy_cable_complete(cable);
}

void ZstStageSession::connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client)
{
	connect_clients(output_client, input_client, [](ZstMessageReceipt receipt) {});
}

void ZstStageSession::connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, const ZstMessageReceivedAction& on_msg_received)
{
	ZstLog::server(LogLevel::notification, "Sending P2P subscribe request to {}", input_client->URI().path());

	//Check to see if one client is already connected to the other
	if (output_client->has_connected_subscriber(input_client)) {
		on_msg_received(ZstMessageReceipt{ Signal_OK });
		return;
	}

	//Create request for receiver
	ZstTransportArgs receiver_args;
	receiver_args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
	receiver_args.on_recv_response = [this, output_client, input_client, on_msg_received](ZstMessageReceipt receipt) {
		if (receipt.status == Signal_OK) {
			complete_client_connection(output_client, input_client);
		}
		else {
			ZstLog::server(LogLevel::warn, "Connection between {} and {} failed: Reason {}", output_client->URI().path(), input_client->URI().path(), EnumNameSignal(receipt.status));
		}
		on_msg_received(receipt);
	};
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto subscribe_offset = CreateClientGraphHandshakeListen(*builder, builder->CreateString(output_client->URI().path()), builder->CreateString(output_client->reliable_address()));
	stage_hierarchy()->whisper(input_client, Content_ClientGraphHandshakeListen, subscribe_offset.Union(), builder, receiver_args);

	//Create request for broadcaster - needs a new buffer builder
	builder = std::make_shared<FlatBufferBuilder>();
	auto broadcast_offset = CreateClientGraphHandshakeStart(
		*builder, 
		builder->CreateString(input_client->URI().path(), input_client->URI().full_size()),
		builder->CreateString(input_client->reliable_address())
	);
	ZstLog::server(LogLevel::notification, "Sending P2P handshake broadcast request to {}", input_client->URI().path());
	ZstTransportArgs broadcaster_args;
	stage_hierarchy()->whisper(output_client, Content_ClientGraphHandshakeStart, broadcast_offset.Union(), builder, broadcaster_args);
}


Signal ZstStageSession::complete_client_connection(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client)
{
	ZstLog::server(LogLevel::notification, "Completing client handshake. Pub: {}, Sub: {}", output_client->URI().path(), input_client->URI().path());

	//Keep a record of which clients are connected to each other
	output_client->add_subscriber(input_client);

	//Let the broadcaster know it can stop publishing messages
	ZstLog::server(LogLevel::notification, "Stopping P2P handshake broadcast from client {}", output_client->URI().path());
	ZstTransportArgs args;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto stop_offset = CreateClientGraphHandshakeStop(*builder, builder->CreateString(input_client->URI().path()) );
	stage_hierarchy()->whisper(output_client, Content_ClientGraphHandshakeStop, stop_offset.Union(), builder, args);

	return Signal_OK;
}

std::shared_ptr<ZstHierarchy> ZstStageSession::hierarchy()
{
	return m_hierarchy;
}

std::shared_ptr<ZstStageHierarchy> ZstStageSession::stage_hierarchy()
{
	return m_hierarchy;
}

}
