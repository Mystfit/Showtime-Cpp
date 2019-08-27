#include "ZstClient.h"

#include "../core/transports/ZstTCPGraphTransport.h"
#ifdef ZST_BUILD_DRAFT_API
#include "../core/transports/ZstUDPGraphTransport.h"
#endif
#include "ZstZMQClientTransport.h"
#include "../core/transports/ZstServiceDiscoveryTransport.h"
#include "../core/ZstMessage.h"
#include "../core/ZstMsgID.h"
#include "../core/ZstPerformanceMessage.h"
#include "../core/ZstSemaphore.h"
#include "../core/ZstZMQRefCounter.h"
#include <fmt/format.h>
#include <czmq.h>

#include "ZstClientSession.h"

namespace Showtime::detail
{
	ZstClient::ZstClient(ShowtimeClient* api) :
		m_ping(-1),
		m_is_ending(false),
		m_is_destroyed(false),
		m_init_completed(false),
		m_connected_to_stage(false),
		m_is_connecting(false),
		m_auto_join_stage(false),

		//Modules
		m_session(std::make_shared<ZstClientSession>()),
		m_heartbeat_timer(m_client_timerloop.IO_context()),
		m_promise_supervisor(ZstMessageSupervisor(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT)),
		m_event_condition(std::make_shared<ZstSemaphore>()),
		m_api(api),

		//Transports
		m_client_transport(std::make_shared<ZstZMQClientTransport>()),
		m_tcp_graph_transport(std::make_shared<ZstTCPGraphTransport>()),
#ifdef ZST_BUILD_DRAFT_API
		m_udp_graph_transport(std::make_shared<ZstUDPGraphTransport>());
#endif
		m_service_broadcast_transport(std::make_shared<ZstServiceDiscoveryTransport>())
	{
		//Register event conditions
		m_service_broadcast_transport->msg_events()->set_wake_condition(m_event_condition);
		m_client_transport->msg_events()->set_wake_condition(m_event_condition);
		m_tcp_graph_transport->msg_events()->set_wake_condition(m_event_condition);
#ifdef ZST_BUILD_DRAFT_API
		m_udp_graph_transport->msg_events()->set_wake_condition(m_event_condition);
#endif
	}

	ZstClient::~ZstClient() {
		destroy();
	}

	void ZstClient::destroy() {
		//Only need to call cleanup once
		if (m_is_ending || m_is_destroyed)
			return;
		set_init_completed(false);

		//Let stage know we are leaving
		if (is_connected_to_stage())
			leave_stage();

		//Since we've sent the leave request, we can flag that we are in the leave process
		set_is_ending(true);

		//Set last status flags
		set_is_ending(false);
		set_is_destroyed(true);

		//Stop timers
		m_client_timerloop.IO_context().stop();
		m_client_timer_thread.interrupt();
		m_client_timer_thread.join();

		//Stop threads
		m_client_event_thread.interrupt();
		m_event_condition->notify();
		m_client_event_thread.try_join_for(boost::chrono::milliseconds(250));
		m_api = NULL;

		m_service_broadcast_transport->destroy();
		m_client_transport->destroy();
		m_tcp_graph_transport->destroy();
#ifdef ZST_BUILD_DRAFT_API
		m_udp_graph_transport->destroy();
#endif

		//All done
		ZstLog::net(LogLevel::notification, "Showtime library destroyed");
	}

	void ZstClient::init_client(const char* client_name, bool debug)
	{
		if (m_is_ending || m_init_completed) {
			ZstLog::net(LogLevel::notification, "Showtime already initialised");
			return;
		}

		set_is_destroyed(false);
		set_is_ending(false);

		LogLevel level = LogLevel::notification;
		if (debug)
			level = LogLevel::debug;

		ZstLog::init_logger(client_name, level);
		ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION_STRING);

		m_client_name = client_name;

		//Todo: init IDs again after stage has responded
		ZstMsgIDManager::init(m_client_name.c_str(), m_client_name.size());

		//Create IO_context thread
		m_client_timer_thread = boost::thread(boost::ref(m_client_timerloop));
		m_connection_timers = ZstConnectionTimerMapUnique(new ZstConnectionTimerMap());
		m_client_timerloop.IO_context().restart();

		//Transport event loops
		m_client_event_thread = boost::thread(boost::bind(&ZstClient::transport_event_loop, this));

		//Register message dispatch as a client adaptor
		ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> >::add_adaptor(m_client_transport);

		//Register adaptors to handle outgoing events
		m_session->init(client_name);
		m_session->stage_events()->add_adaptor(m_client_transport);
		std::static_pointer_cast<ZstClientHierarchy>(m_session->hierarchy())->stage_events()->add_adaptor(m_client_transport);

		//Register module adaptors
		m_session->hierarchy()->hierarchy_events()->add_adaptor(ZstHierarchyAdaptor::downcasted_shared_from_this<ZstHierarchyAdaptor>());

		//Setup adaptors to let transports communicate with client modules
		m_client_transport->init();
		m_client_transport->msg_events()->add_adaptor(ZstTransportAdaptor::downcasted_shared_from_this< ZstTransportAdaptor>());
		m_client_transport->msg_events()->add_adaptor(static_cast<std::shared_ptr<ZstTransportAdaptor> >(m_session));
		m_client_transport->msg_events()->add_adaptor(static_cast<std::shared_ptr<ZstTransportAdaptor> >(std::static_pointer_cast<ZstClientHierarchy>(m_session->hierarchy())));

		//Setup adaptors to receive graph messages
		m_tcp_graph_transport->init();
		m_tcp_graph_transport->msg_events()->add_adaptor(static_cast<std::shared_ptr<ZstTransportAdaptor> >(this));
		m_tcp_graph_transport->msg_events()->add_adaptor(static_cast<std::shared_ptr<ZstTransportAdaptor> >(m_session.get()));

#ifdef ZST_BUILD_DRAFT_API
		m_udp_graph_transport->init();
		m_udp_graph_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(this));
		m_udp_graph_transport->msg_events()->add_adaptor(static_cast<ZstTransportAdaptor*>(m_session.get()));
#endif

		//Stage discovery beacon
		m_service_broadcast_transport->init(STAGE_DISCOVERY_PORT);
		m_service_broadcast_transport->start_listening();
		m_service_broadcast_transport->msg_events()->add_adaptor(static_cast<std::shared_ptr<ZstTransportAdaptor> >(this));

		//Init completed
		set_init_completed(true);
	}

	void ZstClient::init_file_logging(const char* log_file_path)
	{
		ZstLog::init_file_logging(log_file_path);
	}

	void ZstClient::process_events()
	{
		if (!is_init_complete() || m_is_destroyed || m_is_ending) {
			ZstLog::net(LogLevel::debug, "Can't process events until the library is ready");
			return;
		}

		m_session->process_events();
		ZstEventDispatcher< std::shared_ptr<ZstConnectionAdaptor> >::process_events();
		ZstEventDispatcher< std::shared_ptr<ZstTransportAdaptor> >::process_events();
	}

	void ZstClient::flush()
	{
		ZstEventDispatcher< std::shared_ptr<ZstTransportAdaptor >>::flush();
		m_session->flush_events();
	}

	// -----------------------
	// Stage adaptor overrides
	// -----------------------

	void ZstClient::on_receive_msg(ZstMessage* msg)
	{
		auto stage_msg = dynamic_cast<ZstStageMessage*>(msg);
		if (!stage_msg) {
			//Message was not from the stage - check if it is a performance message
			ZstPerformanceMessage* perf_msg = dynamic_cast<ZstPerformanceMessage*>(msg);
			if (perf_msg) {
				receive_connection_handshake(perf_msg);
			}
			return;
		}

		switch (stage_msg->kind()) {
		case ZstMsgKind::START_CONNECTION_HANDSHAKE:
		{
#ifdef ZST_BUILD_DRAFT_API
			m_udp_graph_transport->connect(stage_msg->get_arg<std::string>(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS));
#endif
			std::string input_path_str = stage_msg->get_arg<std::string>(ZstMsgArg::INPUT_PATH);
			start_connection_broadcast(ZstURI(input_path_str.c_str(), input_path_str.size()));
			break;
		}
		case ZstMsgKind::STOP_CONNECTION_HANDSHAKE:
		{
			std::string input_path_str = stage_msg->get_arg<std::string>(ZstMsgArg::INPUT_PATH);
			stop_connection_broadcast(ZstURI(input_path_str.c_str(), input_path_str.size()));
			break;
		}
		case ZstMsgKind::SUBSCRIBE_TO_PERFORMER:
			listen_to_client(stage_msg);
			break;
		case ZstMsgKind::SERVER_BEACON:
			handle_server_discovery(
				stage_msg->get_arg<std::string>(ZstMsgArg::ADDRESS),
				stage_msg->get_arg<std::string>(ZstMsgArg::NAME),
				stage_msg->get_arg<int>(ZstMsgArg::ADDRESS_PORT)
			);
		default:
			break;
		}
	}


	// ------------------------------
	// Performance dispatch overrides
	// ------------------------------

	void ZstClient::receive_connection_handshake(ZstPerformanceMessage* msg)
	{
		std::string output_path_str = msg->sender();
		ZstURI output_path(output_path_str.c_str(), output_path_str.size());
		if (m_pending_peer_connections.find(output_path) != m_pending_peer_connections.end()) {
			ZstLog::net(LogLevel::debug, "Received connection handshake. Msg id {}", m_pending_peer_connections[output_path]);
			ZstTransportArgs args;
			args.msg_args = { {get_msg_arg_name(ZstMsgArg::MSG_ID), m_pending_peer_connections[output_path]} };
			ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> >::invoke([args](std::shared_ptr<ZstTransportAdaptor> adaptor) {
				adaptor->send_msg(ZstMsgKind::OK, args); 
			});
			m_pending_peer_connections.erase(output_path);
		}
	}


	// -------------
	// Client join
	// -------------

	void ZstClient::auto_join_stage(const std::string& name, const ZstTransportRequestBehaviour& sendtype)
	{
		m_auto_join_stage = true;

		// If the server beacon as already been received, join immediately
		for (auto server : m_server_beacons) {
			if (server.name == name) {
				join_stage(server, sendtype);
				return;
			}
		}

		// Create a future that let us wait until a particular server beacon arrives
		ZstMsgID request_id = ZstMsgIDManager::next_id();
		m_auto_join_stage_requests[name] = request_id;
		auto future = m_promise_supervisor.register_response(request_id);
		future.then([this, name, sendtype](ZstMessageFuture f) {
			ZstMessageReceipt receipt;
			try {
				receipt = f.get();

				if (receipt.status != ZstMsgKind::OK) {
					ZstLog::net(LogLevel::error, "Did not receive a server beacon for the named server {}", name);
					return receipt;
				}

				// Only bother connecting if we're in async mode, otherwise the main thread will take care of it
				if (sendtype == ZstTransportRequestBehaviour::ASYNC_REPLY) {
					for (auto server : this->get_discovered_servers()) {
						if (server.name == name) {
							this->join_stage(server, sendtype);
						}
					}
				}
			}
			catch (const ZstTimeoutException& e) {
				ZstLog::net(LogLevel::error, "Did not receive any server beacons - {}", e.what());
				receipt.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
			}
			return receipt;
			});

		if (sendtype == ZstTransportRequestBehaviour::SYNC_REPLY) {
			// Block until beacon is received or we time out
			auto receipt = future.get();
			if (receipt.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::error, "Server autoconnect failed. Status: {}", get_msg_name(receipt.status));
				return;
			}

			// Connect to available named server
			for (auto server : this->get_discovered_servers()) {
				if (server.name == name) {
					this->join_stage(server, sendtype);
					return;
				}
			}

			ZstLog::net(LogLevel::error, "Server autoconnect failed. Could not find server.");
			return;
		}
	}


	void ZstClient::join_stage(const ZstServerAddress& stage_address, const ZstTransportRequestBehaviour& sendtype) {

		if (!m_init_completed) {
			ZstLog::net(LogLevel::error, "Can't join the stage until the library has been initialised");
			return;
		}

		if (m_is_connecting) {
			ZstLog::net(LogLevel::error, "Can't connect to stage, already connecting");
			return;
		}

		if (m_is_connecting || m_connected_to_stage) {
			ZstLog::net(LogLevel::error, "Can't connect to stage, already connected");
			return;
		}
		set_is_connecting(true);

		ZstLog::net(LogLevel::notification, "Connecting to stage {}", stage_address.address);
		m_client_transport->connect(stage_address.address);

		//Acquire our output graph address to send to the stage
		std::string reliable_graph_addr = m_tcp_graph_transport->get_graph_out_address();
#ifdef ZST_BUILD_DRAFT_API
		std::string unreliable_graph_addr = m_udp_graph_transport->get_graph_in_address();
#else
		std::string unreliable_graph_addr = "";
#endif

		//Activate any child entities and factories that were added to the root performer already
		ZstPerformer* root = session()->hierarchy()->get_local_performer();

		//Construct transport args
		ZstTransportArgs args;
		args.msg_args = {
			{ get_msg_arg_name(ZstMsgArg::GRAPH_RELIABLE_OUTPUT_ADDRESS), reliable_graph_addr },
			{ get_msg_arg_name(ZstMsgArg::GRAPH_UNRELIABLE_INPUT_ADDRESS), unreliable_graph_addr }
		};
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this, stage_address](ZstMessageReceipt response) { this->join_stage_complete(stage_address, response); };
		root->write_json(args.msg_payload);

		//Send message
		ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor>>::invoke([this, args, stage_address](std::shared_ptr<ZstTransportAdaptor> adaptor) {
			adaptor->send_msg(ZstMsgKind::CLIENT_JOIN, args);
		});
	}

	void ZstClient::handle_server_discovery(const std::string& address, const std::string& server_name, int port)
	{
		// Make a server address to hold our server name/address pair
		ZstServerAddress server = ZstServerAddress(server_name, fmt::format("{}:{}", address, port));

		// Add server to list of discovered servers if the beacon hasn't been seen before
		if (m_server_beacons.find(server) == m_server_beacons.end()) {
			ZstLog::net(LogLevel::debug, "Received new server beacon: {} {}", server.name, server.address);

			// Store server beacon
			m_server_beacons.insert(server);
			ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor>>::defer([this, server](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
				adaptor->on_server_discovered(this->m_api, server);
			});

			// Handle connecting to the stage automatically
			if (m_auto_join_stage && !is_connected_to_stage()) {
				auto server_search_request = m_auto_join_stage_requests.find(server.name);
				if (server_search_request != m_auto_join_stage_requests.end()) {
					m_promise_supervisor.process_response(server_search_request->second, ZstMessageReceipt{ ZstMsgKind::OK });
				}
			}
		}
	}

	const ZstServerList& ZstClient::get_discovered_servers()
	{
		return m_server_beacons;
	}


	void ZstClient::join_stage_complete(const ZstServerAddress& server_address, ZstMessageReceipt response)
	{
		set_is_connecting(false);
		set_connected_to_stage(true);
		m_connected_server = server_address;

		//If we didn't receive a OK signal, something went wrong
		if (response.status != ZstMsgKind::OK) {
			ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", get_msg_name(response.status));
			leave_stage_complete();
			return;
		}

		ZstLog::net(LogLevel::notification, "Connection to server established");

		//Add local entities to entity lookup and attach adaptors only if we've connected to the stage
		ZstEntityBundle bundle;
		m_session->hierarchy()->get_local_performer()->get_factories(bundle);
		m_session->hierarchy()->get_local_performer()->get_child_entities(bundle, true);
		for (auto c : bundle) {
			c->synchronisable_events()->add_adaptor(static_cast<std::shared_ptr< ZstSynchronisableAdaptor> >(m_session->hierarchy()));
			c->entity_events()->add_adaptor(static_cast<std::shared_ptr<ZstEntityAdaptor> >(m_session->hierarchy()));
			synchronisable_set_activating(c);
			synchronisable_enqueue_activation(c);
		}

		//Enqueue connection events
		m_session->dispatch_connected_to_stage();

		//Set up heartbeat timer
		m_heartbeat_timer.expires_from_now(boost::posix_time::milliseconds(HEARTBEAT_DURATION));
		m_heartbeat_timer.async_wait(boost::bind(&ZstClient::heartbeat_timer, &m_heartbeat_timer, this, boost::posix_time::milliseconds(HEARTBEAT_DURATION)));

		//Ask the stage to send us the current session
		synchronise_graph(response.request_behaviour);

		//Enqueue connection events
		m_session->dispatch_connected_to_stage();
		ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor> >::defer([this, server_address](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
			adaptor->on_connected_to_stage(this->m_api, server_address);
		});

		//If we are sync, we can dispatch events immediately
		if (response.request_behaviour == ZstTransportRequestBehaviour::SYNC_REPLY)
			process_events();
	}

	void ZstClient::synchronise_graph(const ZstTransportRequestBehaviour& sendtype)
	{
		ZstLog::net(LogLevel::notification, "Requesting graph snapshot");

		//Build message
		ZstTransportArgs args;
		args.msg_send_behaviour = sendtype;
		args.on_recv_response = [this](ZstMessageReceipt response) {
			if (response.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::warn, "Failed to synchronise client with server. Reason: {}", get_msg_name(response.status));
				return;
			}
			this->synchronise_graph_complete(response);
		};

		//Send message
		ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor>>::invoke([&args](std::shared_ptr<ZstTransportAdaptor> adaptor) {
			adaptor->send_msg(ZstMsgKind::CLIENT_SYNC, args);
		});
	}

	void ZstClient::synchronise_graph_complete(ZstMessageReceipt response)
	{
		ZstLog::net(LogLevel::notification, "Graph sync completed");
		ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor>>::defer([this](std::shared_ptr<ZstConnectionAdaptor> adp) {
			adp->on_synchronised_with_stage(this->m_api, this->connected_server());
		});
	}

	void ZstClient::leave_stage()
	{
		if (m_connected_to_stage) {
			ZstLog::net(LogLevel::notification, "Leaving stage");

			//Set flags early to avoid double leaving shenanigans
			this->set_is_connecting(false);
			this->set_connected_to_stage(false);

			ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor>>::invoke([](std::shared_ptr<ZstTransportAdaptor> adaptor) {
				adaptor->send_msg(ZstMsgKind::CLIENT_LEAVING); 
			});
		}
		else {
			ZstLog::net(LogLevel::debug, "Not connected to stage. Skipping to cleanup. {}");
		}

		this->leave_stage_complete();
		this->process_events();
	}

	void ZstClient::leave_stage_complete()
	{
		//Set stage as disconnected again - just to make sure
		set_connected_to_stage(false);

		//Remove root performer from entity lookup
		ZstEntityBundle bundle;
		m_session->hierarchy()->get_local_performer()->get_factories(bundle);
		m_session->hierarchy()->get_local_performer()->get_child_entities(bundle, true);
		for (auto c : bundle) {
			m_session->hierarchy()->remove_entity_from_lookup(c);
		}

		//Disconnect rest of sockets and timers
		boost::system::error_code ec;
		m_heartbeat_timer.cancel(ec);
		//m_heartbeat_timer.wait();
		ZstLog::net(LogLevel::debug, "Timer cancel status: {}", ec.message());
		if (m_client_transport)
			m_client_transport->disconnect();

		//Enqueue event for adaptors
		m_session->dispatch_disconnected_from_stage();
		ZstEventDispatcher<std::shared_ptr<ZstConnectionAdaptor>>::invoke([this](std::shared_ptr<ZstConnectionAdaptor> adaptor) {
			if(m_api)
				adaptor->on_disconnected_from_stage(this->m_api, this->m_connected_server);
		});

		m_connected_server = ZstServerAddress();
	}

	const ZstServerAddress& ZstClient::connected_server()
	{
		return m_connected_server;
	}

	bool ZstClient::is_connected_to_stage()
	{
		return m_connected_to_stage;
	}

	bool ZstClient::is_connecting_to_stage()
	{
		return m_is_connecting;
	}

	bool ZstClient::is_init_complete() {
		return m_init_completed;
	}

	long ZstClient::ping()
	{
		return m_ping;
	}

	void ZstClient::heartbeat_timer(boost::asio::deadline_timer* t, ZstClient* client, boost::posix_time::milliseconds duration) {
		if (!client->is_connected_to_stage())
			return;

		//Build message
		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
		ZstTransportArgs args;
		args.msg_send_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
		args.on_recv_response = [client, start](ZstMessageReceipt response) {
			if (!client)
				return;

			if (response.status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::warn, "Server ping timed out");
				client->leave_stage_complete();
				return;
			}
			std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
			auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			client->m_ping = static_cast<long>(delta.count());
		};

		//Send message
		client->ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor>>::invoke([client, args](std::shared_ptr<ZstTransportAdaptor> adaptor) {
			adaptor->send_msg(ZstMsgKind::CLIENT_HEARTBEAT, args); 
		});

		//Loop timer
		t->expires_at(t->expires_at() + duration);
		t->async_wait(boost::bind(&ZstClient::heartbeat_timer, t, client, duration));
	}

	void ZstClient::start_connection_broadcast(const ZstURI& remote_client_path)
	{
		ZstPerformer* local_client = session()->hierarchy()->get_local_performer();
		ZstPerformer* remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
		ZstLog::net(LogLevel::debug, "Starting peer handshake broadcast to {}", remote_client->URI().path());

		if (!remote_client) {
			ZstLog::net(LogLevel::error, "Could not find performer {}", remote_client_path.path());
			return;
		}

		boost::asio::deadline_timer timer(m_client_timerloop.IO_context(), boost::posix_time::milliseconds(100));
		m_connection_timers->insert({ remote_client->URI(),  std::move(timer) });
		m_connection_timers->at(remote_client->URI()).async_wait(boost::bind(
			&ZstClient::send_connection_broadcast,
			&m_connection_timers->at(remote_client->URI()),
			this,
			remote_client->URI(),
			local_client->URI(),
			boost::posix_time::milliseconds(100)
		));
	}

	void ZstClient::send_connection_broadcast(boost::asio::deadline_timer* t, ZstClient* client, const ZstURI& to, const ZstURI& from, boost::posix_time::milliseconds duration)
	{
		ZstLog::net(LogLevel::debug, "Sending connection handshake. From: {}, To: {}", from.path(), to.path());

		ZstTransportArgs args;
		args.msg_args = { { get_msg_arg_name(ZstMsgArg::PATH), from.path() } };
		client->m_tcp_graph_transport->send_msg(ZstMsgKind::CONNECTION_HANDSHAKE, args);

		if (client->m_connection_timers->find(to) != client->m_connection_timers->end()) {
			//Loop timer if it is valid
			t->expires_at(t->expires_at() + duration);
			t->async_wait(boost::bind(&ZstClient::send_connection_broadcast, t, client, to, from, duration));
		}
	}

	void ZstClient::stop_connection_broadcast(const ZstURI& remote_client_path)
	{
		ZstPerformer* remote_client = dynamic_cast<ZstPerformer*>(session()->hierarchy()->find_entity(remote_client_path));
		ZstLog::net(LogLevel::debug, "Stopping peer handshake broadcast to {}", remote_client->URI().path());

		if (!remote_client) {
			ZstLog::net(LogLevel::error, "Could not find performer {}", remote_client_path.path());
			return;
		}

		if (m_connection_timers->find(remote_client->URI()) != m_connection_timers->end()) {
			m_connection_timers->at(remote_client->URI()).cancel();
			m_connection_timers->at(remote_client->URI()).wait();
			m_connection_timers->erase(remote_client->URI());
		}
	}

	void ZstClient::listen_to_client(ZstMessage* msg)
	{
		ZstStageMessage* stage_msg = static_cast<ZstStageMessage*>(msg);
		std::string output_path_str = stage_msg->get_arg<std::string>(ZstMsgArg::OUTPUT_PATH);
		std::string graph_out_addr = stage_msg->get_arg<std::string>(ZstMsgArg::GRAPH_RELIABLE_OUTPUT_ADDRESS);

		ZstLog::net(LogLevel::debug, "Listening to performer {}", output_path_str);
		m_pending_peer_connections[ZstURI(output_path_str.c_str(), output_path_str.size())] = stage_msg->id();
		m_tcp_graph_transport->connect(graph_out_addr);
	}

	void ZstClient::transport_event_loop()
	{
		while (1) {
			try {
				boost::this_thread::interruption_point();
				m_event_condition->wait();
				if (this->m_is_destroyed)
					break;

				// Process events from transports
				m_client_transport->process_events();
#ifdef ZST_BUILD_DRAFT_API
				m_udp_graph_transport->process_events();
#endif
				m_tcp_graph_transport->process_events();
				m_service_broadcast_transport->process_events();

				// Process events to transports
				ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> >::process_events();

				// Process promise responses
				m_promise_supervisor.cleanup_response_messages();
			}
			catch (boost::thread_interrupted) {
				break;
			}
		}
	}

	std::shared_ptr<ZstClientSession> ZstClient::session()
	{
		return m_session;
	}

	void ZstClient::set_is_ending(bool value)
	{
		//std::lock_guard<std::mutex> lock(m_event_loop_mutex);
		m_is_ending = value;
	}

	void ZstClient::set_is_destroyed(bool value)
	{
		//std::lock_guard<std::mutex> lock(m_event_loop_mutex);
		m_is_destroyed = value;
	}

	void ZstClient::set_init_completed(bool value)
	{
		//std::lock_guard<std::mutex> lock(m_event_loop_mutex);
		m_init_completed = value;
	}

	void ZstClient::set_connected_to_stage(bool value)
	{
		//std::lock_guard<std::mutex> lock(m_event_loop_mutex);
		m_connected_to_stage = value;
	}

	void ZstClient::set_is_connecting(bool value)
	{
		//std::lock_guard<std::mutex> lock(m_event_loop_mutex);
		m_is_connecting = value;
	}


	void ZstClient::on_entity_arriving(ZstEntityBase* entity)
	{
		ZstEntityBundle bundle;
		entity->get_child_entities(bundle);

		for (auto child : bundle) {
			// Arriving output plugs need to register the graph transport so that they can dispatch messages
			if (strcmp(child->entity_type(), PLUG_TYPE) == 0) {
				init_arriving_plug(static_cast<ZstPlug*>(child));
			}
		}
	}

	void ZstClient::on_performer_arriving(ZstPerformer* performer)
	{
		on_entity_arriving(performer);
	}

	void ZstClient::init_arriving_plug(ZstPlug* plug)
	{
		ZstOutputPlug* output_plug = dynamic_cast<ZstOutputPlug*>(plug);
		if (output_plug) {
			std::shared_ptr<ZstGraphTransport> transport = NULL;
			if (output_plug->is_reliable()) {
				transport = std::static_pointer_cast<ZstGraphTransport>(m_tcp_graph_transport);
			}
#ifdef ZST_BUILD_DRAFT_API
			else {
				transport = m_udp_graph_transport;
			}
#endif
			// Setup plug as fireable if we own it
			if (session()->hierarchy()->get_local_performer()->URI() == plug->URI().first()) {
				output_plug_set_can_fire(output_plug, true);
			}

			// Attach plug transport so output plugs can send messages to the graph
			output_plug_set_transport(output_plug, std::static_pointer_cast<ZstTransportAdaptor>(transport));
		}
	}
}
// -------------------


