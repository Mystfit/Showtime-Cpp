#include "ZstStageSession.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace boost::uuids;

ZstStageSession::ZstStageSession() : m_hierarchy(std::make_shared<ZstStageHierarchy>())
{
}

ZstStageSession::~ZstStageSession()
{
    ZstCableBundle bundle;
    for (auto cable : get_cables(bundle)) {
        destroy_cable(cable);
    }
}

void ZstStageSession::destroy()
{
}

void ZstStageSession::process_events()
{
	ZstStageModule::process_events();
	ZstSession::process_events();
	hierarchy()->process_events();
}

void ZstStageSession::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
    ZstStageModule::set_wake_condition(condition);
    m_hierarchy->set_wake_condition(condition);
    session_events()->set_wake_condition(condition);
    synchronisable_events()->set_wake_condition(condition);
}

void ZstStageSession::on_receive_msg(ZstMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

	ZstMsgKind response(ZstMsgKind::EMPTY);
	ZstPerformerStageProxy * sender = m_hierarchy->get_client_from_endpoint_UUID(msg->endpoint_UUID());

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	switch (stage_msg->kind()) {
	case ZstMsgKind::CLIENT_SYNC:
		response = synchronise_client_graph_handler(stage_msg, sender);
		break;
	case ZstMsgKind::CREATE_CABLE:
		response = create_cable_handler(stage_msg, sender);
		break;
	case ZstMsgKind::DESTROY_CABLE:
		response = destroy_cable_handler(stage_msg);
		break;
	case ZstMsgKind::OBSERVE_ENTITY:
		response = observe_entity_handler(stage_msg, sender);
		break;
	case ZstMsgKind::AQUIRE_ENTITY_OWNERSHIP:
		response = aquire_entity_ownership_handler(stage_msg, sender);
		break;
	case ZstMsgKind::RELEASE_ENTITY_OWNERSHIP:
		response = release_entity_ownership_handler(stage_msg, sender);
		break;
	case ZstMsgKind::CLIENT_HEARTBEAT:
		sender->set_heartbeat_active();
		response = ZstMsgKind::OK;
		break;
	default:
		break;
	}

	//Return response to sender
	if (response != ZstMsgKind::EMPTY) {
		ZstTransportArgs args;
		args.target_endpoint_UUID = msg->endpoint_UUID();
		args.msg_args = { { get_msg_arg_name(ZstMsgArg::MSG_ID), stage_msg->id() } };
		router_events()->defer([response, args](std::shared_ptr<ZstTransportAdaptor> adaptor) { 
			adaptor->send_msg(response, args);
		});
	}
}

ZstMsgKind ZstStageSession::synchronise_client_graph_handler(ZstStageMessage * msg, ZstPerformer * sender) {

	ZstLog::server(LogLevel::notification, "Sending graph snapshot to {}", sender->URI().path());

	//Send performer root entities
	ZstEntityBundle bundle;
	for (auto performer : hierarchy()->get_performers(bundle)) {
		//Only pack performers that aren't the destination client
		if (performer->URI() != sender->URI()) {
			ZstTransportArgs args;
			performer->write_json(args.msg_payload);
			stage_hierarchy()->whisper_message(static_cast<ZstPerformer*>(sender), ZstMsgKind::CREATE_PERFORMER, args);
		}
	}

	//Send cables
	for (auto const & cable : m_cables) {
		ZstTransportArgs args;
		cable->get_address().write_json(args.msg_payload);
		stage_hierarchy()->whisper_message(static_cast<ZstPerformer*>(sender), ZstMsgKind::CREATE_CABLE, args);
	}

	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::create_cable_handler(ZstStageMessage* msg, ZstPerformerStageProxy * sender)
{
	//Unpack cable from message
	ZstCableAddress cable_path = msg->unpack_payload_serialisable<ZstCableAddress>();
	ZstLog::server(LogLevel::notification, "Received connect cable request for In:{} and Out:{}", cable_path.get_input_URI().path(), cable_path.get_output_URI().path());

	//Make sure cable doesn't already exist
	if (find_cable(cable_path)) {
        ZstLog::server(LogLevel::warn, "Cable {}<-{} already exists", cable_path.get_input_URI().path(), cable_path.get_output_URI().path());
		return ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Verify plugs will accept cable
	ZstInputPlug * input = dynamic_cast<ZstInputPlug*>(hierarchy()->find_entity(cable_path.get_input_URI()));
	ZstOutputPlug * output = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(cable_path.get_output_URI()));
	if (!input || !output) {
		return ZstMsgKind::ERR_CABLE_PLUGS_NOT_FOUND;
	}

	//Unplug existing cables if we hit max cables in an input plug
	if (input->num_cables() >= input->max_connected_cables()){
		ZstLog::server(LogLevel::warn, "Too many cables in plug. Disconnecting existing cables");
		ZstCableBundle bundle;
        input->get_child_cables(bundle);
		for (auto cable : bundle) {
			destroy_cable(cable);
		}
	}

	//Create our local cable ptr
	auto cable = create_cable(input , output);
	if (!cable) {
		return ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Start the client connection
	auto input_perf = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(input->URI().first()));
	auto output_perf = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(output->URI().first()));
	ZstTransportArgs args;
	args.msg_args = { { get_msg_arg_name(ZstMsgArg::MSG_ID), msg->id()} };
	connect_clients(output_perf, input_perf, [this, cable, args, sender](ZstMessageReceipt receipt) {
		if (receipt.status == ZstMsgKind::OK) {
			create_cable_complete_handler(cable);
		}
		stage_hierarchy()->whisper_message(sender, receipt.status, args);
	});

	return ZstMsgKind::EMPTY;
}


//---------------------

ZstMsgKind ZstStageSession::observe_entity_handler(ZstStageMessage * msg, ZstPerformerStageProxy * sender)
{
	//Get target performer
	std::string performer_path_str = msg->get_arg<std::string>(ZstMsgArg::OUTPUT_PATH);
	ZstURI performer_path(performer_path_str.c_str(), performer_path_str.size());
	ZstPerformerStageProxy * observed_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(performer_path));
	if (!observed_performer) {
		return ZstMsgKind::ERR_STAGE_PERFORMER_NOT_FOUND;
	}
	
	ZstLog::server(LogLevel::notification, "Received observation request. Requestor: {}, Observed: {}", sender->URI().path(), observed_performer->URI().path());
	if (sender == observed_performer) {
		ZstLog::server(LogLevel::warn, "Client attempting to observe itself");
		return ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_CONNECTED;
	}

	//Check to see if one client is already connected to the other
	if (observed_performer->has_connected_subscriber(sender)) {
		ZstLog::server(LogLevel::warn, "Client {} already observing {}", sender->URI().path(), observed_performer->URI().path());
		return ZstMsgKind::ERR_STAGE_PERFORMER_ALREADY_CONNECTED;
	}

	//Start the client connection
	ZstTransportArgs args;
	args.msg_ID = msg->id();
	connect_clients(observed_performer, sender, [this, sender, args](ZstMessageReceipt receipt) {
		stage_hierarchy()->whisper_message(sender, receipt.status, args);
	});

	return ZstMsgKind::EMPTY;
}


ZstMsgKind ZstStageSession::aquire_entity_ownership_handler(ZstStageMessage* msg, ZstPerformerStageProxy* sender)
{
	auto entity_path = ZstURI(msg->get_arg<std::string>(ZstMsgArg::PATH).c_str());
	ZstEntityBase* entity = hierarchy()->find_entity(entity_path);

	if (!entity) {
		ZstLog::server(LogLevel::warn, "Could not aquire entity ownership - entity {}, not found", entity_path.path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}
    
    ZstLog::server(LogLevel::notification, "Received entity ownership aquistion request - {} wants to control {}", sender->URI().path(), entity->URI().path());

	//Set owner
	entity_set_owner(entity, sender->URI());

	// We need to connect downstream plugs to the new sender
	ZstCableBundle bundle;
	get_cables(bundle);

	std::unordered_map<ZstURI, ZstPerformerStageProxy*, ZstURIHash> performers;
	
	//Find all performers that have input connections to this output plug
	for (auto c : bundle) {
		if (c->get_output()->URI() == entity->URI()) {
			auto receiver = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->get_performer_by_URI(c->get_input()->URI().first()));
			performers[receiver->URI()] = receiver;
		}
	}

	//Connect performers together that will have to update their subscriptions
	for (auto receiver : performers) {
		connect_clients(receiver.second, sender);
	}

	//Broadcast change in plug fire control
	ZstLog::server(LogLevel::notification, "Broadcasting entity ownership - {} controls {}", sender->URI().path(), entity->URI().path());
	ZstTransportArgs args;
	args.msg_args = {
		{ get_msg_arg_name(ZstMsgArg::PATH), entity->URI().path() },
		{ get_msg_arg_name(ZstMsgArg::OUTPUT_PATH), sender->URI().path() }
	};
	stage_hierarchy()->broadcast_message(ZstMsgKind::AQUIRE_ENTITY_OWNERSHIP, args);

	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::release_entity_ownership_handler(ZstStageMessage* msg, ZstPerformerStageProxy* sender)
{
	auto entity_path = ZstURI(msg->get_arg<std::string>(ZstMsgArg::PATH).c_str());
	ZstEntityBase* entity = hierarchy()->find_entity(entity_path);

	if (!entity) {
		ZstLog::server(LogLevel::warn, "Could not release entity ownership - entity {}, not found", entity_path.path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	ZstLog::server(LogLevel::notification, "Received entity ownership release request - {} wants to release ownership", sender->URI().path());

	//Reset owner
	entity_set_owner(entity, "");

	//Broadcast an empty path for the entity owner to reset ownership to the creator of the entity
	ZstTransportArgs args;
	args.msg_args = {
		{ get_msg_arg_name(ZstMsgArg::PATH), entity->URI().path() },
		{ get_msg_arg_name(ZstMsgArg::OUTPUT_PATH), entity->get_owner().path() }
	};
	stage_hierarchy()->broadcast_message(ZstMsgKind::AQUIRE_ENTITY_OWNERSHIP, args);
    
    return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::create_cable_complete_handler(ZstCable * cable)
{
	ZstLog::server(LogLevel::notification, "Client connection complete. Publishing cable {}", cable->get_address().to_string());
	ZstTransportArgs args;
	cable->get_address().write_json(args.msg_payload);
	stage_hierarchy()->broadcast_message(ZstMsgKind::CREATE_CABLE, args);
	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::destroy_cable_handler(ZstStageMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	auto cable_path = stage_msg->unpack_payload_serialisable<ZstCableAddress>();
	ZstLog::server(LogLevel::notification, "Received destroy cable connection request");

	ZstCable * cable_ptr = find_cable(cable_path);
	destroy_cable(cable_ptr);

	return ZstMsgKind::OK;
}

void ZstStageSession::on_performer_leaving(ZstPerformer * performer)
{
	disconnect_cables(performer);
}

void ZstStageSession::on_entity_leaving(ZstEntityBase * entity)
{
	disconnect_cables(entity);
}

void ZstStageSession::on_plug_leaving(ZstPlug * plug)
{
	disconnect_cables(plug);
}

void ZstStageSession::disconnect_cables(ZstEntityBase * entity)
{
	ZstCableBundle bundle;
    entity->get_child_cables(bundle);
	for (auto cable : bundle) {
		destroy_cable(cable);
	}
}

void ZstStageSession::destroy_cable(ZstCable * cable) {
	if (!cable)
		return;

	ZstLog::server(LogLevel::notification, "Destroying cable {}", cable->get_address().to_string());

	//Update rest of network
	ZstTransportArgs args;
	cable->get_address().write_json(args.msg_payload);
	stage_hierarchy()->broadcast_message(ZstMsgKind::DESTROY_CABLE, args);

	//Remove cable
	ZstSession::destroy_cable_complete(cable);
}

void ZstStageSession::connect_clients(ZstPerformerStageProxy* output_client, ZstPerformerStageProxy* input_client)
{
	connect_clients(output_client, input_client, [](ZstMessageReceipt receipt){});
}

void ZstStageSession::connect_clients(ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client, const ZstMessageReceivedAction & on_msg_received)
{
	ZstLog::server(LogLevel::notification, "Sending P2P subscribe request to {}", input_client->URI().path());

	//Check to see if one client is already connected to the other
	if (output_client->has_connected_subscriber(input_client)) {
		on_msg_received(ZstMessageReceipt{ZstMsgKind::OK});
		return;
	}

	//Create request for receiver
	ZstTransportArgs receiver_args;
	receiver_args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
	receiver_args.on_recv_response = [this, output_client, input_client, on_msg_received](ZstMessageReceipt receipt) {
		if (receipt.status == ZstMsgKind::OK) {
			complete_client_connection(output_client, input_client);
			on_msg_received(receipt);
		}
	};
	receiver_args.msg_args = {
		{ get_msg_arg_name(ZstMsgArg::OUTPUT_PATH), output_client->URI().path() },
		{ get_msg_arg_name(ZstMsgArg::GRAPH_RELIABLE_OUTPUT_ADDRESS), output_client->reliable_address() },
	};

	//Create request for broadcaster
	ZstLog::server(LogLevel::notification, "Sending P2P handshake broadcast request to {}", output_client->URI().path());
	ZstTransportArgs broadcaster_args;
	broadcaster_args.msg_args = { 
		{ get_msg_arg_name(ZstMsgArg::INPUT_PATH), input_client->URI().path() },
		{ get_msg_arg_name(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS), input_client->unreliable_address() },
	};

	//Send messages
	stage_hierarchy()->whisper_message(input_client, ZstMsgKind::SUBSCRIBE_TO_PERFORMER, receiver_args);
	stage_hierarchy()->whisper_message(output_client, ZstMsgKind::START_CONNECTION_HANDSHAKE, broadcaster_args);
}


ZstMsgKind ZstStageSession::complete_client_connection(ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client)
{
	ZstLog::server(LogLevel::notification, "Completing client handshake. Pub: {}, Sub: {}", output_client->URI().path(), input_client->URI().path());

	//Keep a record of which clients are connected to each other
	output_client->add_subscriber(input_client);

	//Let the broadcaster know it can stop publishing messages
	ZstLog::server(LogLevel::notification, "Stopping P2P handshake broadcast from client {}", output_client->URI().path());
	ZstTransportArgs args;
	args.msg_args = { { get_msg_arg_name(ZstMsgArg::INPUT_PATH), input_client->URI().path()} };
	stage_hierarchy()->whisper_message(output_client, ZstMsgKind::STOP_CONNECTION_HANDSHAKE, args);
	return ZstMsgKind::OK;
}

std::shared_ptr<ZstHierarchy> ZstStageSession::hierarchy()
{
	return m_hierarchy;
}

std::shared_ptr<ZstStageHierarchy> ZstStageSession::stage_hierarchy()
{
	return m_hierarchy;
}
