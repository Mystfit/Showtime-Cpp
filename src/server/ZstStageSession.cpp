#include "ZstStageSession.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "../core/transports/ZstStageTransport.h"

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

void ZstStageSession::set_wake_condition(std::shared_ptr<std::condition_variable>& condition)
{
	ZstStageModule::set_wake_condition(condition);
	m_hierarchy->set_wake_condition(condition);
	session_events()->set_wake_condition(condition);
	synchronisable_events()->set_wake_condition(condition);
}

void ZstStageSession::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg)
{
	Signal response = Signal_EMPTY;
	ZstPerformerStageProxy* sender = m_hierarchy->get_client_from_endpoint_UUID(msg->origin_endpoint_UUID());

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (msg->type()) {
	case Content_SignalMessage:
		response = signal_handler(msg, sender);
		break;
	case Content_CableCreateRequest:
		response = create_cable_handler(msg, sender);
		break;
	case Content_CableDestroyRequest:
		response = destroy_cable_handler(msg);
		break;
	case Content_EntityObserveRequest:
		response = observe_entity_handler(msg, sender);
		break;
	case Content_EntityTakeOwnershipRequest:
		response = aquire_entity_ownership_handler(msg, sender);
		break;
	default:
		break;
	}

	//Return response to sender
	if (response != Signal_EMPTY) {
		ZstTransportArgs args;
		args.target_endpoint_UUID = msg->origin_endpoint_UUID();
		args.msg_ID = msg->id();
		auto builder = std::make_shared< FlatBufferBuilder>();
		auto signal_offset = CreateSignalMessage(*builder, response);
		if (auto transport = std::dynamic_pointer_cast<ZstStageTransport>(msg->owning_transport()))
			transport->send_msg(Content_SignalMessage, signal_offset.Union(), builder, args);
	}
}

Signal ZstStageSession::signal_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender)
{
	auto request = msg->buffer()->content_as_SignalMessage();

	if (!sender) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	if (request->signal() == Signal_CLIENT_SYNC) {
		return synchronise_client_graph_handler(sender);
	}

	return Signal_EMPTY;
}

Signal ZstStageSession::synchronise_client_graph_handler(ZstPerformerStageProxy* sender) 
{
	Log::server(Log::Level::notification, "Sending graph snapshot to {}", sender->URI().path());

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
			performer->get_child_entities(&entity_bundle, true, true);
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

Signal ZstStageSession::create_cable_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender)
{
	auto request = msg->buffer();
	auto cable_request = request->content_as_CableCreateRequest();

	//Unpack cable from message
	auto input_path = ZstURI(cable_request->cable()->address()->input_URI()->c_str(), cable_request->cable()->address()->input_URI()->size());
	auto output_path = ZstURI(cable_request->cable()->address()->output_URI()->c_str(), cable_request->cable()->address()->output_URI()->size());
	auto cable_path = ZstCableAddress(input_path, output_path);
	Log::server(Log::Level::notification, "Received connect cable request for In:{} and Out:{}", cable_path.get_input_URI().path(), cable_path.get_output_URI().path());

	//Make sure cable doesn't already exist
	if (find_cable(cable_path)) {
		Log::server(Log::Level::warn, "Cable {}<-{} already exists", input_path.path(), output_path.path());
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
		Log::server(Log::Level::warn, "Too many cables in plug. Disconnecting existing cables");
		ZstCableBundle bundle;
		input->get_child_cables(&bundle);
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

	auto connect_finished_cb = [this, cable_ptr, sender, id = msg->id()](Signal signal) {
		if (signal == Signal_OK) {
			Log::server(Log::Level::notification, "Client connection complete. Publishing cable {}", cable_ptr->get_address().to_string());
			ZstTransportArgs args;
			auto builder = std::make_shared<FlatBufferBuilder>();

			// Publish cable
			auto cable_create_offset = CreateCableCreateRequest(*builder, cable_ptr->get_address().serialize(*builder));
			stage_hierarchy()->broadcast(Content_CableCreateRequest, cable_create_offset.Union(), builder, args);
		}

		// Let original requestor know the request was completed
		stage_hierarchy()->reply_with_signal(sender, signal, id);
	};

	auto connect_response_cb = [id = msg->id(), connect_finished_cb](ZstMessageResponse response) {
		connect_finished_cb(ZstStageTransport::get_signal(response.response));
	};

	// If both the input and output performer are the same then messages will be routed internally for that client
	if (output_perf->URI() == input_perf->URI())
		connect_finished_cb(Signal_OK);
	else
		connect_clients(output_perf, input_perf, connect_response_cb);

	return Signal_EMPTY;
}


//---------------------

Signal ZstStageSession::observe_entity_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender)
{
	//Get target performer
	auto request = msg->buffer();
	auto observe_request = request->content_as_EntityObserveRequest();
	auto performer_path = ZstURI(observe_request->URI()->c_str(), observe_request->URI()->size());
	ZstPerformerStageProxy* observed_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(performer_path.first()));
	if (!observed_performer) {
		return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
	}

	Log::server(Log::Level::notification, "Received observation request. Requestor: {}, Observed: {}", sender->URI().path(), observed_performer->URI().path());
	if (sender == observed_performer) {
		Log::server(Log::Level::warn, "Client attempting to observe itself");
		return Signal_ERR_STAGE_PERFORMER_ALREADY_CONNECTED;
	}

	//Check to see if one client is already connected to the other
	if (observed_performer->has_connected_subscriber(sender)) {
		Log::server(Log::Level::warn, "Client {} already observing {}", sender->URI().path(), observed_performer->URI().path());
		return Signal_ERR_STAGE_PERFORMER_ALREADY_CONNECTED;
	}

	//Check to see if one client is already connected to the other
	if (!observed_performer->has_connected_subscriber(sender)) {
		//Start the client connection
		connect_clients(observed_performer, sender, [this, sender, response_id = msg->id()](ZstMessageResponse response) {
			auto signal = ZstStageTransport::get_signal(response.response);
			stage_hierarchy()->reply_with_signal(sender, signal, response_id);
		});
		return Signal_EMPTY;
	}

	return Signal_OK;
}


Signal ZstStageSession::aquire_entity_ownership_handler(const std::shared_ptr<ZstStageMessage>& msg, ZstPerformerStageProxy* sender)
{
	auto request = msg->buffer()->content_as_EntityTakeOwnershipRequest();
	auto entity_path = ZstURI(request->URI()->c_str(), request->URI()->size());
	ZstEntityBase* entity = hierarchy()->find_entity(entity_path);

	if (!entity) {
		Log::server(Log::Level::warn, "Could not aquire entity ownership - entity {}, not found", entity_path.path());
		return Signal_ERR_ENTITY_NOT_FOUND;
	}

	ZstURI new_owner_path;
	ZstPerformerStageProxy* new_owner = NULL;

	// An empty owner means we return ownership back to the original owner
	if (!request->new_owner()->size()){
		entity_set_owner(entity, "");
	}
	else {
		new_owner_path = ZstURI(request->new_owner()->c_str(), request->new_owner()->size());
		new_owner = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(new_owner_path));
		if (!new_owner) {
			Log::server(Log::Level::warn, "Could not aquire entity ownership - could not find new owner {}", new_owner_path.path());
			return Signal_ERR_STAGE_PERFORMER_NOT_FOUND;
		}

		Log::server(Log::Level::notification, "Received entity ownership aquistion request - {} wants to control {}", new_owner_path.path(), entity->URI().path());

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
	Log::server(Log::Level::notification, "Broadcasting entity ownership - {} controls {}", new_owner_path.path(), entity->URI().path());
	ZstTransportArgs args;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto ownership_offset = CreateEntityTakeOwnershipRequest(*builder, builder->CreateString(entity->URI().path()), builder->CreateString(new_owner_path.path()));
	stage_hierarchy()->broadcast(Content_EntityTakeOwnershipRequest, ownership_offset.Union(), builder, args);

	return Signal_OK;
}

Signal ZstStageSession::destroy_cable_handler(const std::shared_ptr<ZstStageMessage>& msg)
{
	auto request = msg->buffer()->content_as_CableDestroyRequest();

	auto cable_path = ZstCableAddress(request->cable());
	Log::server(Log::Level::notification, "Received destroy cable connection request");

	ZstCable* cable_ptr = find_cable(cable_path);
	destroy_cable(cable_ptr);
	
	return Signal_OK;
}

void ZstStageSession::on_performer_leaving(const ZstURI& performer_path)
{
	disconnect_cables(m_hierarchy->find_entity(performer_path));
}

void ZstStageSession::on_entity_leaving(const ZstURI& entity_path)
{
	disconnect_cables(m_hierarchy->find_entity(entity_path));
}

void ZstStageSession::disconnect_cables(ZstEntityBase* entity)
{
	if (!entity)
		return;

	ZstCableBundle bundle;
 	entity->get_child_cables(&bundle);
	for (auto cable : bundle) {
		destroy_cable(cable);
	}
}

void ZstStageSession::destroy_cable(ZstCable* cable) {
	if (!cable)
		return;

	Log::server(Log::Level::notification, "Destroying cable {}", cable->get_address().to_string());

	//Update rest of network
	ZstTransportArgs args;
	auto builder = std::make_shared<FlatBufferBuilder>();
	auto destroy_cable_data_offset = CreateCableData(*builder, builder->CreateString(cable->get_address().get_input_URI().path()), builder->CreateString(cable->get_address().get_output_URI().path()));
	auto destroy_cable_offset = CreateCableDestroyRequest(*builder, CreateCable(*builder, destroy_cable_data_offset));
	stage_hierarchy()->broadcast(Content_CableDestroyRequest, destroy_cable_offset.Union(), builder, args);

	//Remove cable
	ZstSession::destroy_cable_complete(cable);
}

void ZstStageSession::connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client)
{
	connect_clients(output_client, input_client, [](ZstMessageResponse response) {});
}

void ZstStageSession::connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client, const ZstMessageReceivedAction& on_msg_received)
{
	Log::server(Log::Level::notification, "Sending P2P subscribe request to {}", input_client->URI().path());

	//Check to see if one client is already connected to the other
	if (output_client->has_connected_subscriber(input_client)) {
		return;
	}

	//If the output and input clients are the same, then plug messages will be routed internally
	if (output_client->URI() == input_client->URI()) {
		return;
	}

	//Create request for receiver
	auto output_address = output_client->URI();
	auto input_address = input_client->URI();
	ZstTransportArgs receiver_args;
	receiver_args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
	receiver_args.on_recv_response = [this, output_client, output_address, input_client, input_address, on_msg_received](ZstMessageResponse response) {
		if (ZstStageTransport::verify_signal(response.response, Signal_OK, fmt::format("P2P client connection ({} --> {})", output_address.path(), input_address.path()))) {
			complete_client_connection(output_client, input_client);
			on_msg_received(response);
		}
	};

	auto builder = std::make_shared<FlatBufferBuilder>();
	auto subscribe_offset = CreateClientGraphHandshakeListen(*builder, builder->CreateString(output_client->URI().path()), builder->CreateString(output_client->reliable_address()), builder->CreateString(output_client->unreliable_address()), builder->CreateString(output_client->unreliable_public_address()));

	Log::net(Log::Level::debug, "Sending Content_ClientGraphHandshakeListen whisper to {}", input_client->URI().path());
	stage_hierarchy()->whisper(input_client, Content_ClientGraphHandshakeListen, subscribe_offset.Union(), builder, receiver_args);

	//Create request for broadcaster - needs a new buffer builder
	auto broadcaster_builder = std::make_shared<FlatBufferBuilder>();
	auto broadcast_offset = CreateClientGraphHandshakeStart(
		*broadcaster_builder,
		broadcaster_builder->CreateString(input_client->URI().path(), input_client->URI().full_size()),
		broadcaster_builder->CreateString(input_client->unreliable_address()),
		broadcaster_builder->CreateString(input_client->unreliable_public_address())
	);
	ZstTransportArgs broadcaster_args;
	stage_hierarchy()->whisper(output_client, Content_ClientGraphHandshakeStart, broadcast_offset.Union(), broadcaster_builder, broadcaster_args);
}


Signal ZstStageSession::complete_client_connection(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client)
{
	Log::server(Log::Level::notification, "Completing client handshake. Pub: {}, Sub: {}", output_client->URI().path(), input_client->URI().path());

	//Keep a record of which clients are connected to each other
	output_client->add_subscriber(input_client);

	//Let the broadcaster know it can stop publishing messages
	Log::server(Log::Level::notification, "Stopping P2P handshake broadcast from client {}", output_client->URI().path());
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
