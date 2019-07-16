#include "ZstStageSession.h"
#include <cf/cfuture.h>


ZstStageSession::ZstStageSession() :
	m_connection_watcher(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT),
    m_hierarchy(NULL)
{
	m_hierarchy = new ZstStageHierarchy();
}

ZstStageSession::~ZstStageSession()
{
	delete m_hierarchy;
}

void ZstStageSession::destroy()
{
	ZstCableBundle bundle;
	for (auto cable : get_cables(bundle)) {
		destroy_cable(cable);
	}

	hierarchy()->destroy();
	ZstSession::destroy();
    m_connection_watcher.destroy();
}

void ZstStageSession::process_events()
{
	ZstSession::process_events();
	m_connection_watcher.cleanup_response_messages();
}

void ZstStageSession::set_wake_condition(std::weak_ptr<ZstSemaphore> condition)
{
    ZstStageModule::set_wake_condition(condition);
    m_hierarchy->set_wake_condition(condition);
    session_events().set_wake_condition(condition);
    synchronisable_events().set_wake_condition(condition);
}

void ZstStageSession::on_receive_msg(ZstMessage * msg)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

	ZstMsgKind response(ZstMsgKind::EMPTY);
	std::string sender_identity = stage_msg->get_arg<std::string>(ZstMsgArg::SENDER);
	ZstPerformerStageProxy * sender = m_hierarchy->get_client_from_socket_id(sender_identity);

	//Check client hasn't finished joining yet
	if (!sender)
		return;

	//Process connection responses first to avoid fresh promises getting insta-resolved
	m_connection_watcher.process_response(stage_msg->id(), stage_msg->kind());

	switch (stage_msg->kind()) {
	case ZstMsgKind::CLIENT_SYNC:
		response = synchronise_client_graph(sender);
		break;
	case ZstMsgKind::CREATE_CABLE:
		response = create_cable_handler(msg, sender);
		break;
	case ZstMsgKind::DESTROY_CABLE:
		response = destroy_cable_handler(msg);
		break;
	case ZstMsgKind::OBSERVE_ENTITY:
		response = observe_entity_handler(msg, sender);
		break;
	case ZstMsgKind::AQUIRE_PLUG_FIRE_CONTROL:
		response = aquire_plug_fire_control_handler(msg, sender);
		break;
	case ZstMsgKind::RELEASE_PLUG_FIRE_CONTROL:
		response = release_plug_fire_control_handler(msg, sender);
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
		router_events().invoke([response, &sender_identity, &stage_msg](ZstTransportAdaptor * adp) {
			adp->send_msg(response, {
				{ get_msg_arg_name(ZstMsgArg::DESTINATION), sender_identity },
				{ get_msg_arg_name(ZstMsgArg::MSG_ID), stage_msg->id() }
			});
		});
	}
}

ZstMsgKind ZstStageSession::synchronise_client_graph(ZstPerformer * client) {

	ZstLog::server(LogLevel::notification, "Sending graph snapshot to {}", client->URI().path());

	//Create sender args
	ZstMsgArgs args{ { get_msg_arg_name(ZstMsgArg::DESTINATION), this->hierarchy()->get_socket_ID(client) } };

	//Send performer root entities
	ZstEntityBundle bundle;
	for (auto performer : hierarchy()->get_performers(bundle)) {
		//Only pack performers that aren't the destination client
		if (performer->URI() != client->URI()) {
			router_events().invoke([&performer, &args](ZstTransportAdaptor * adp) {
				adp->send_msg(ZstMsgKind::CREATE_PERFORMER, args, performer->as_json());
			});
		}
	}

	//Send cables
	for (auto const & cable : m_cables) {
		router_events().invoke([&cable, &args](ZstTransportAdaptor * adp) {
			adp->send_msg(ZstMsgKind::CREATE_CABLE, args, cable->get_address().as_json());
		});
	}

	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::create_cable_handler(ZstMessage * msg, ZstPerformerStageProxy * sender)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

	//Unpack cable from message
	ZstCableAddress cable_path = stage_msg->unpack_payload_serialisable<ZstCableAddress>();
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
	ZstCable * cable_ptr = create_cable(input, output);
	if (!cable_ptr) {
		return ZstMsgKind::ERR_STAGE_BAD_CABLE_CONNECT_REQUEST;
	}

	//Create connection request for the entity who owns the input plug
	ZstPerformerStageProxy * input_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(cable_ptr->get_address().get_input_URI().first()));
	ZstPerformerStageProxy * output_performer = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->find_entity(cable_ptr->get_address().get_output_URI().first()));

	//Check to see if one client is already connected to the other
	if (output_performer->has_connected_subscriber(input_performer)) {
		return create_cable_complete_handler(cable_ptr);
	}

	ZstMsgID id = stage_msg->id();		
	auto future = m_connection_watcher.register_response(id);

	//Create future action to run when the client responds
	future.then([this, cable_ptr, sender, id, input_performer, output_performer](ZstMessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			ZstMsgKind status = f.get();
			if (status != ZstMsgKind::OK) {
				ZstLog::server(LogLevel::error, "Client failed to complete P2P connection with status {}", get_msg_name(status));
				return status;
			}

			//Finish connection setup
			complete_client_connection(output_performer, input_performer);

			//Publish cable to graph
			create_cable_complete_handler(cable_ptr);

			//Let caller know the operation has successfully completed
			router_events().invoke([id, this, sender](ZstTransportAdaptor * adp) {
				adp->send_msg(ZstMsgKind::OK, { 
					{get_msg_arg_name(ZstMsgArg::DESTINATION), this->m_hierarchy->get_socket_ID(sender)},
					{get_msg_arg_name(ZstMsgArg::MSG_ID), id }
				});
			});
			
			return status;
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::server(LogLevel::error, "Client connection async response timed out - {}", e.what());
		}
		return status;
	});

	//Start the client connection
	connect_clients(id, output_performer, input_performer);
	
	return ZstMsgKind::EMPTY;
}

//---------------------

ZstMsgKind ZstStageSession::observe_entity_handler(ZstMessage * msg, ZstPerformerStageProxy * sender)
{
	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);

	//Get target performer
	std::string performer_path_str = stage_msg->get_arg<std::string>(ZstMsgArg::OUTPUT_PATH);
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

	//Prepare connection promises
	ZstMsgID id = stage_msg->id();
	auto future = m_connection_watcher.register_response(id);

	//Create future action to run when the client responds
	future.then([this, sender, observed_performer, id](ZstMessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			ZstMsgKind status = f.get();
			if (status == ZstMsgKind::OK) {
				ZstLog::server(LogLevel::notification, "Observation request completed. Requester: {}, Observed: {} completed", sender->URI().path(), observed_performer->URI().path());

				//Finish connection setup
				complete_client_connection(observed_performer, sender);

				//Let caller know the operation has successfully completed
				router_events().invoke([id, this, sender](ZstTransportAdaptor * adp) {
					adp->send_msg(ZstMsgKind::OK, {
						{ get_msg_arg_name(ZstMsgArg::DESTINATION), this->m_hierarchy->get_socket_ID(sender) },
						{ get_msg_arg_name(ZstMsgArg::MSG_ID), id }
					});
				});
			}
			return status;
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::server(LogLevel::error, "Client connection async response timed out - {}", e.what());
		}
		return status;
	});

	//Start the client connection
	connect_clients(id, observed_performer, sender);

	return ZstMsgKind::EMPTY;
}


ZstMsgKind ZstStageSession::aquire_plug_fire_control_handler(ZstMessage* msg, ZstPerformerStageProxy* sender)
{
	ZstStageMessage* stage_msg = static_cast<ZstStageMessage*>(msg);
	auto plug_path = ZstURI(stage_msg->get_arg<std::string>(ZstMsgArg::PATH).c_str());
	ZstOutputPlug* plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(plug_path));

	if (!plug) {
		ZstLog::server(LogLevel::warn, "Could not aquire plug fire control - plug {}, not found {}", plug_path.path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	//Prepare connection promises
	ZstMsgID id = stage_msg->id();
	auto future = m_connection_watcher.register_response(id);

	// We need to connect downstream plugs to the new sender
	ZstCableBundle bundle;
	get_cables(bundle);

	std::unordered_map<ZstURI, ZstPerformerStageProxy*, ZstURIHash> performers;
	
	//Find all performers that have input connections to this output plug
	for (auto c : bundle) {
		if (c->get_output()->URI() == plug->URI()) {
			auto receiver = dynamic_cast<ZstPerformerStageProxy*>(hierarchy()->get_performer_by_URI(c->get_input()->URI().first()));
			performers[receiver->URI()] = receiver;
		}
	}

	for (auto receiver : performers) {
		//Check to see if one client is already connected to the other
		if (sender->has_connected_subscriber(receiver.second)) {
			continue;
		}

		//Create future action to run when the client responds
		future.then([this, plug, sender, receiver, id](ZstMessageFuture f) {
			ZstMsgKind status(ZstMsgKind::EMPTY);
			try {
				ZstMsgKind status = f.get();
				if (status == ZstMsgKind::OK) {
					ZstLog::server(LogLevel::notification, "aquire_plug_fire_control request completed. Requester: {}, Observed: {} completed", sender->URI().path(), receiver.second->URI().path());

					//Finish connection setup
					complete_client_connection(receiver.second, sender);

					//Let caller know the operation has successfully completed
					router_events().invoke([id, this, sender](ZstTransportAdaptor* adp) {
						adp->send_msg(ZstMsgKind::OK, {
							{ get_msg_arg_name(ZstMsgArg::DESTINATION), this->m_hierarchy->get_socket_ID(sender) },
							{ get_msg_arg_name(ZstMsgArg::MSG_ID), id }
							});
						});
				}
				return status;
			}
			catch (const ZstTimeoutException& e) {
				ZstLog::server(LogLevel::error, "Client connection async response timed out - {}", e.what());
			}
			return status;
			});

		//Start the client connection
		connect_clients(id, receiver.second, sender);
	}

	//Broadcast change in plug fire control
	ZstMsgArgs args {
		{ get_msg_arg_name(ZstMsgArg::PATH), plug->URI().path() },
		{ get_msg_arg_name(ZstMsgArg::OUTPUT_PATH), sender->URI().path() }
	};
	m_hierarchy->broadcast_message(ZstMsgKind::AQUIRE_PLUG_FIRE_CONTROL, args);

	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::release_plug_fire_control_handler(ZstMessage* msg, ZstPerformerStageProxy* sender)
{
	ZstStageMessage* stage_msg = static_cast<ZstStageMessage*>(msg);
	auto plug_path = ZstURI(stage_msg->get_arg<std::string>(ZstMsgArg::PATH).c_str());
	ZstOutputPlug* plug = dynamic_cast<ZstOutputPlug*>(hierarchy()->find_entity(plug_path));

	if (!plug) {
		ZstLog::server(LogLevel::warn, "Could not release plug fire control - plug {}, not found {}", plug_path.path());
		return ZstMsgKind::ERR_ENTITY_NOT_FOUND;
	}

	//Broadcast an empty path for the fire control owner to reset ownership to the creator of the plug
	m_hierarchy->broadcast_message(ZstMsgKind::AQUIRE_PLUG_FIRE_CONTROL, {{ get_msg_arg_name(ZstMsgArg::PATH), "" }});
}

//---------------------


ZstMsgKind ZstStageSession::create_cable_complete_handler(ZstCable * cable)
{
	ZstLog::server(LogLevel::notification, "Client connection complete. Publishing cable {} -> {}", cable->get_address().get_input_URI().path(), cable->get_address().get_output_URI().path());
	m_hierarchy->broadcast_message(ZstMsgKind::CREATE_CABLE, json(), cable->get_address().as_json());
	return ZstMsgKind::OK;
}

ZstMsgKind ZstStageSession::destroy_cable_handler(ZstMessage * msg)
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

	ZstLog::server(LogLevel::notification, "Destroying cable {} -> {}", cable->get_address().get_output_URI().path(), cable->get_address().get_input_URI().path());

	//Update rest of network
	m_hierarchy->broadcast_message(ZstMsgKind::DESTROY_CABLE, json(), cable->get_address().as_json());

	//Remove cable
	ZstSession::destroy_cable_complete(cable);
}


void ZstStageSession::connect_clients(const ZstMsgID & response_id, ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client)
{
	ZstLog::server(LogLevel::notification, "Sending P2P subscribe request to {}", input_client->URI().path());

	//Attach URI and IP of output client
	ZstMsgArgs receiver_args{
		{ get_msg_arg_name(ZstMsgArg::OUTPUT_PATH), output_client->URI().path() },
		{ get_msg_arg_name(ZstMsgArg::GRAPH_RELIABLE_OUTPUT_ADDRESS), output_client->reliable_address() },
		{ get_msg_arg_name(ZstMsgArg::REQUEST_ID), response_id },
		{ get_msg_arg_name(ZstMsgArg::DESTINATION), m_hierarchy->get_socket_ID(input_client) }
	};
	
	router_events().invoke([&receiver_args](ZstTransportAdaptor * adp) {
		adp->send_msg(ZstMsgKind::SUBSCRIBE_TO_PERFORMER, receiver_args);
	});

	ZstLog::server(LogLevel::notification, "Sending P2P handshake broadcast request to {}", output_client->URI().path());
	ZstMsgArgs broadcaster_args{ 
		{ get_msg_arg_name(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS), input_client->unreliable_address() },
		{ get_msg_arg_name(ZstMsgArg::DESTINATION), m_hierarchy->get_socket_ID(output_client) },
		{ get_msg_arg_name(ZstMsgArg::INPUT_PATH), input_client->URI().path() }
	};
	router_events().invoke([&broadcaster_args](ZstTransportAdaptor * adp) {
		adp->send_msg(ZstMsgKind::START_CONNECTION_HANDSHAKE, broadcaster_args);
	});
}


ZstMsgKind ZstStageSession::complete_client_connection(ZstPerformerStageProxy * output_client, ZstPerformerStageProxy * input_client)
{
	ZstLog::server(LogLevel::notification, "Completing client handshake. Pub: {}, Sub: {}", output_client->URI().path(), input_client->URI().path());

	//Keep a record of which clients are connected to each other
	output_client->add_subscriber(input_client);

	//Let the broadcaster know it can stop publishing messages
	ZstLog::server(LogLevel::notification, "Stopping P2P handshake broadcast from client {}", output_client->URI().path());
	ZstMsgArgs args{ 
		{ get_msg_arg_name(ZstMsgArg::INPUT_PATH), input_client->URI().path()},
		{ get_msg_arg_name(ZstMsgArg::DESTINATION), m_hierarchy->get_socket_ID(output_client)}
	};
	router_events().invoke([&args](ZstTransportAdaptor * adp) {
		adp->send_msg(ZstMsgKind::STOP_CONNECTION_HANDSHAKE, args);
	});
	return ZstMsgKind::OK;
}

ZstStageHierarchy * ZstStageSession::hierarchy()
{
	return m_hierarchy;
}
