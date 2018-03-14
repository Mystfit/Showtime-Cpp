#include <chrono>
#include <sstream>

#include "ZstClient.h"

using namespace std;

ZstClient::ZstClient() :
    m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false),
    m_graph_out_ip(""),
	m_root(NULL),
    m_num_graph_recv_messages(0),
    m_num_graph_send_messages(0)
{
	//Event queues

	//Client events
	m_synchronisable_deferred_event = new ZstSynchronisableDeferredEvent();
	m_performer_leaving_hook = new ZstEntityLeavingEvent();
	m_component_leaving_hook = new ZstEntityLeavingEvent();
	m_cable_leaving_hook = new ZstCableLeavingEvent();
	m_plug_leaving_hook = new ZstPlugLeavingEvent();
	m_compute_event = new ZstComputeEvent();
	
	m_synchronisable_event_manager.attach_event_listener(m_synchronisable_deferred_event);
	m_compute_event_manager.attach_event_listener(m_compute_event);
	
	m_performer_leaving_event_manager.attach_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager.attach_post_event_callback(m_component_leaving_hook);
	m_cable_leaving_event_manager.attach_post_event_callback(m_cable_leaving_hook);
	m_plug_leaving_event_manager.attach_post_event_callback(m_plug_leaving_hook);

	//Message pools
	m_message_pool.populate(MESSAGE_POOL_BLOCK);
}

ZstClient::~ZstClient() {
	destroy();

	m_synchronisable_event_manager.remove_event_listener(m_synchronisable_deferred_event);
	m_compute_event_manager.remove_event_listener(m_compute_event);

	m_performer_leaving_event_manager.remove_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager.remove_post_event_callback(m_component_leaving_hook);
	m_cable_leaving_event_manager.remove_post_event_callback(m_cable_leaving_hook);
	m_plug_leaving_event_manager.remove_post_event_callback(m_plug_leaving_hook);
	
	delete m_synchronisable_deferred_event;
	delete m_performer_leaving_hook;
	delete m_component_leaving_hook;
	delete m_cable_leaving_hook;
	delete m_plug_leaving_hook;
	delete m_compute_event;
}

ZstClient & ZstClient::instance()
{
	static ZstClient client_singleton;
	return client_singleton;
}

void ZstClient::destroy() {
	//Only need to call cleanup once
	if (m_is_ending || m_is_destroyed)
		return;
    m_is_ending = true;
    m_init_completed = false;
    
    //Let stage know we are leaving
	leave_stage();
    
    //TODO: Delete proxies and templates
    delete m_root;
    
	ZstActor::destroy();
	zsock_destroy(&m_stage_updates);
	zsock_destroy(&m_stage_router);
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
	zsys_shutdown();
	m_is_ending = false;
	m_is_destroyed = true;
}

void ZstClient::init_client(const char *client_name, bool debug)
{
	if (m_is_ending || m_init_completed) {
		return;
	}

	ZstLog::init_logger(client_name);
	ZstLog::net(LogLevel::notification, "Starting Showtime v{}", SHOWTIME_VERSION);

	ZstActor::init(client_name);
	m_client_name = client_name;
	m_is_destroyed = false;
	
	m_startup_uuid = zuuid_new();

	//Local dealer socket for receiving messages forwarded from other performers
	m_stage_router = zsock_new(ZMQ_DEALER);
	if (m_stage_router) {
		zsock_set_linger(m_stage_router, 0);
		attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
	}

	//Graph input socket
	m_graph_in = zsock_new(ZMQ_SUB);
	if (m_graph_in) {
		zsock_set_linger(m_graph_in, 0);
		zsock_set_unbounded(m_graph_in);
		attach_pipe_listener(m_graph_in, s_handle_graph_in, this);
	}

	//Stage update socket
	m_stage_updates = zsock_new(ZMQ_SUB);
	if (m_stage_updates) {
		zsock_set_linger(m_stage_updates, 0);
		attach_pipe_listener(m_stage_updates, s_handle_stage_update_in, this);
	}
    
    string network_ip = first_available_ext_ip();
    ZstLog::net(LogLevel::notification, "Using external IP {}", network_ip);
    
	//Graph output socket
	std::stringstream addr;
	addr << "@tcp://" << network_ip.c_str() << ":*";
    m_graph_out = zsock_new_pub(addr.str().c_str());
	m_graph_out_addr = zsock_last_endpoint(m_graph_out);
	addr.str("");

	//Set graph socket as unbounded by HWM
	zsock_set_unbounded(m_graph_out);
	char * output_ip = zsock_last_endpoint(m_graph_out);
    m_graph_out_ip = std::string(output_ip);
	zstr_free(&output_ip);
	ZstLog::net(LogLevel::notification, "Client graph address: {}", m_graph_out_ip);
    
    if(m_graph_out)
        zsock_set_linger(m_graph_out, 0);
    
    //Set up outgoing sockets
    std::string identity = std::string(zuuid_str_canonical(m_startup_uuid));
	ZstLog::net(LogLevel::notification, "Setting socket identity to {}. Length {}", identity, identity.size());
    
    zsock_set_identity(m_stage_router, identity.c_str());
	
	//Create a root entity to hold our local entity hierarchy
	m_root = new ZstPerformer(actor_name(), m_graph_out_addr.c_str());
	m_root->set_network_interactor(this);
	
    start();
    
    m_init_completed = true;
}

void ZstClient::init_file_logging(const char * log_file_path)
{
	ZstLog::init_file_logging(log_file_path);
}

void ZstClient::process_callbacks()
{
    if(!is_init_complete() || m_is_destroyed){
        return;
    }
    
    m_client_connected_event_manager.process();
    m_compute_event_manager.process();
    m_synchronisable_event_manager.process();
    m_client_disconnected_event_manager.process();
    m_cable_arriving_event_manager.process();
    m_cable_leaving_event_manager.process();
    m_plug_arriving_event_manager.process();
    m_plug_leaving_event_manager.process();
    m_component_arriving_event_manager.process();
    m_component_leaving_event_manager.process();
    m_component_type_arriving_event_manager.process();
    m_component_type_leaving_event_manager.process();
    m_performer_arriving_event_manager.process();
    m_performer_leaving_event_manager.process();

	//Only delete entities after all callbacks have finished
	m_reaper.reap_all();
}

void ZstClient::flush_events()
{
	m_synchronisable_event_manager.flush();
	m_client_connected_event_manager.flush();
	m_client_disconnected_event_manager.flush();
    m_plug_arriving_event_manager.flush();
    m_plug_leaving_event_manager.flush();
	m_cable_arriving_event_manager.flush();
	m_cable_leaving_event_manager.flush();
	m_component_arriving_event_manager.flush();
	m_component_leaving_event_manager.flush();
	m_component_type_arriving_event_manager.flush();
	m_component_type_leaving_event_manager.flush();
    m_performer_arriving_event_manager.flush();
    m_performer_leaving_event_manager.flush();
	m_compute_event_manager.flush();
}

string ZstClient::first_available_ext_ip(){
    ziflist_t * interfaces = ziflist_new();
	string interface_ip_str = "127.0.0.1";

    if(ziflist_first(interfaces) != NULL) {
		interface_ip_str = string(ziflist_address(interfaces));
    }

    return interface_ip_str;
}

void ZstClient::start() {
	ZstActor::start();
}

void ZstClient::stop() {
	ZstActor::stop();
}


// ------------
// Send/Receive
// ------------

void ZstClient::publish(ZstPlug * plug) 
{
	zmsg_t *msg = zmsg_new();

	//Add output plug path
	zmsg_addstr(msg, plug->URI().path());

	//Pack value into stream
	std::stringstream s;
	plug->raw_value()->write(s);
	zframe_t *payload = zframe_new(s.str().c_str(), s.str().size());
	zmsg_append(msg, &payload);

	//Send it
	zmsg_send(&msg, m_graph_out);
	m_num_graph_send_messages++;
}

void ZstClient::send_to_stage(ZstMessage * msg) 
{
	//Dealer socket doesn't add an empty frame to seperate identity chain and payload, so we handle it here
	zframe_t * empty = zframe_new_empty();
	zmsg_t * msg_handle = msg->handle();
	zmsg_prepend(msg_handle, &empty);
	msg->send(m_stage_router);
	msg_pool().release(msg);
}

ZstMessage * ZstClient::receive_stage_update() 
{
	ZstMessage * msg = NULL;
	zmsg_t * recv_msg = zmsg_recv(m_stage_updates);
	if (recv_msg) {
		msg = msg_pool().get();
		msg->unpack(recv_msg);
	}
	return msg;
}

ZstMessage * ZstClient::receive_from_stage() {
	ZstMessage * msg = NULL;

	zmsg_t * recv_msg = zmsg_recv(m_stage_router);
	if (recv_msg) {
		msg = msg_pool().get();

		//Pop blank seperator frame left from the dealer socket
		zframe_t * empty = zmsg_pop(recv_msg);
		zframe_destroy(&empty);

		//Unpack message contents
		msg->unpack(recv_msg);
	}
	return msg;
}


// ------------
// Message pools
// ------------

ZstMessagePool & ZstClient::msg_pool()
{
	return m_message_pool;
}


// -------------
// Client init
// -------------

void ZstClient::register_client_to_stage(std::string stage_address, bool async) {
	
	if (m_is_connecting || m_connected_to_stage) {
		ZstLog::net(LogLevel::error, "Connection in progress or already connected, can't connect to stage.");
		return;
	}
	m_is_connecting = true;
	
	ZstLog::net(LogLevel::notification, "Connecting to stage {}", stage_address);
	m_stage_addr = string(stage_address);

    stringstream addr;
    addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
    m_stage_router_addr = addr.str();
    
    zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
    addr.str("");
    
    //Stage subscriber socket for update messages
    addr << "tcp://" << m_stage_addr << ":" << STAGE_PUB_PORT;
    m_stage_updates_addr = addr.str();
    
	ZstLog::net(LogLevel::notification, "Connecting to stage publisher {}", m_stage_updates_addr);
    zsock_connect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
    zsock_set_subscribe(m_stage_updates, "");
    addr.str("");
    
	//Build connect message
	ZstMessage * msg = msg_pool().get()->init_serialisable_message(ZstMsgKind::CLIENT_JOIN, *m_root);
	
	MessageFuture future = msg_pool().register_future(msg, true);
	if(async){
		register_client_to_stage_async(future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		register_client_to_stage_sync(future);
	}
}

void ZstClient::register_client_to_stage_sync(MessageFuture & future)
{
	ZstMsgKind status(ZstMsgKind::EMPTY);
	try {
		status = future.get();
		register_client_complete(status);
		synchronise_graph(false);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::error, fmt::format("Stage sync join timed out - {}", e.what()).c_str());
		leave_stage_complete();
		status = ZstMsgKind::ERR_STAGE_TIMEOUT;
	}
}

void ZstClient::register_client_to_stage_async(MessageFuture & future)
{
	future.then([this](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			status = f.get();
			this->register_client_complete(status);
			this->synchronise_graph(true);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::error, fmt::format("Stage async join timed out - {}", e.what()).c_str());
			leave_stage_complete();
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::register_client_complete(ZstMsgKind status)
{
	m_is_connecting = false;

	//If we didn't receive a OK signal, something went wrong
	if (status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Stage connection failed with with status: {}", status);
        return;
	}

	ZstLog::net(LogLevel::notification, "Connection to server established");

	//Set up heartbeat timer
	m_heartbeat_timer_id = attach_timer(s_heartbeat_timer, HEARTBEAT_DURATION, this);

	//TODO: Need a handshake with the stage before we mark connection as active
	m_connected_to_stage = true;
}
          
void ZstClient::synchronise_graph(bool async)
{
    if(!is_connected_to_stage()){
        ZstLog::net(LogLevel::error, "Can't synchronise graph if we're not connected");
        return;
    }
    
    //Ask the stage to send us a full snapshot
    ZstLog::net(LogLevel::notification, "Requesting stage snapshot");
    ZstMessage * msg = msg_pool().get()->init_message(ZstMsgKind::CLIENT_SYNC);
	MessageFuture future = msg_pool().register_future(msg, true);
	
	if (async) {
		synchronise_graph_async(future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		synchronise_graph_sync(future);
	}
}

void ZstClient::synchronise_graph_sync(MessageFuture & future)
{
	try {
		ZstMsgKind status = future.get();
		this->synchronise_graph_complete(status);
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Synchronising graph sync with stage timed out: {}", e.what());
	}
}

void ZstClient::synchronise_graph_async(MessageFuture & future)
{
	future.then([this](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY); 
		try {
			status = f.get();
			this->synchronise_graph_complete(status);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::notification, "Synchronising graph async with stage timed out: {}", e.what());
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::synchronise_graph_complete(ZstMsgKind status)
{
   	m_root->enqueue_activation();
    client_connected_events().enqueue(m_root);
    ZstLog::net(LogLevel::notification, "Graph sync completed");
}

void ZstClient::leave_stage(bool immediately)
{
	if (m_connected_to_stage) {
		ZstLog::net(LogLevel::notification, "Leaving stage");
        
        ZstMessage * msg = msg_pool().get()->init_message(ZstMsgKind::CLIENT_LEAVING);
		//If leaving immediately then don't stick around
		if (immediately) {
			send_to_stage(msg);
			leave_stage_complete();
			return;
		}

		//Leave slowly by waiting for leave OK from the stage
		MessageFuture future = msg_pool().register_future(msg, true);
		send_to_stage(msg);

		try {
			future.get();
			leave_stage_complete();
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::notification, "Stage leave timeout: {}", e.what());
		}
    } else {
        ZstLog::net(LogLevel::debug, "Not connected to stage. Skipping to cleanup. {}");
        leave_stage_complete();
    }
}

void ZstClient::leave_stage_complete()
{
    //Deactivate all owned entities
	m_root->enqueue_deactivation();
    
    //Purge callbacks
    process_callbacks();

    //Disconnect rest of sockets and timers
	detach_timer(m_heartbeat_timer_id);
    zsock_disconnect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
    zsock_disconnect(m_stage_router, "%s", m_stage_router_addr.c_str());

	m_is_connecting = false;
	m_connected_to_stage = false;
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


// ---------------
// Socket handlers
// ---------------
int ZstClient::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
	ZstClient *client = (ZstClient*)arg;

	//Receive message from graph
	zmsg_t *msg = zmsg_recv(client->m_graph_in);

	if (client->graph_message_handler(msg) < 0) {
		//Errored
	}
	
    zmsg_destroy(&msg);

	return 0;
}

int ZstClient::graph_message_handler(zmsg_t * msg) {
	ZstLog::net(LogLevel::debug, "Received graph message");

	//Get sender from msg
	//zframe_t * sender_frame = zmsg_pop(msg);
	char * sender_c = zmsg_popstr(msg);
	ZstURI sender(sender_c);
	zstr_free(&sender_c);

	//Get payload from msg
	zframe_t * payload = zmsg_pop(msg);

	//Find local proxy of the0 sneding plug
	ZstPlug * sending_plug = dynamic_cast<ZstPlug*>(find_entity(sender));
	ZstInputPlug * receiving_plug = NULL;

	if (!sending_plug) {
		ZstLog::net(LogLevel::warn, "No sending plug found");
		return -1;
	}

	//Iterate over all connected cables from the sending plug
	for (auto cable : *sending_plug) {
		receiving_plug = dynamic_cast<ZstInputPlug*>(cable->get_input());
		if (receiving_plug) {
			if (entity_is_local(*receiving_plug)) {
				ZstLog::net(LogLevel::debug, "Reading incoming plug value into {}", receiving_plug->URI().path());
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				receiving_plug->raw_value()->read((char*)zframe_data(payload), zframe_size(payload), offset);
				
				ZstLog::net(LogLevel::debug, "Enqueing compute event for plug {}", receiving_plug->URI().path());
				compute_events().enqueue(receiving_plug);
			}
		}
	}

	//Telemetrics
	m_num_graph_recv_messages++;

	//Cleanup
	zframe_destroy(&payload);

	return 0;
}

int ZstClient::s_handle_stage_update_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstClient *client = (ZstClient*)arg;
	ZstMessage * msg = client->receive_stage_update();
	client->stage_update_handler(msg);
	return 0;
}

int ZstClient::s_handle_stage_router(zloop_t * loop, zsock_t * socket, void * arg){
	ZstClient *client = (ZstClient*)arg;

	//Receive routed message from stage
    ZstMessage * msg = client->receive_from_stage();

    //Process messages addressed to our client specifically
    if (msg->kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
        ZstLog::net(LogLevel::notification, "Received graph snapshot");
        //Handle graph snapshot synchronisation
        client->stage_update_handler(msg);
    }
    else if (msg->kind() == ZstMsgKind::SUBSCRIBE_TO_PERFORMER) {
        //Handle connection requests from other clients
        client->connect_client_handler(msg->payload_at(0).data(), msg->payload_at(1).data());
    } else if (msg->kind() == ZstMsgKind::OK){
        //Do nothing?
    } else {
        ZstLog::net(LogLevel::notification, "Stage router sent unknown message {}", msg->kind());
    }
    
    //Process message promises
    client->msg_pool().process_message_promise(msg);
    
    //Cleanup
	client->msg_pool().release(msg);
    return 0;
}

void ZstClient::stage_update_handler(ZstMessage * msg)
{
	ZstMsgKind payload_kind = msg->kind();

	for (size_t i = 0; i < msg->num_payloads(); ++i)
	{
		if (msg->kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
			payload_kind = msg->payload_at(i).kind();
		}
		else {
			payload_kind = (msg->kind());
		}

		//Do something with payload
		switch (payload_kind) {
		case ZstMsgKind::CREATE_CABLE:
		{
			const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(i);
			//Only dispatch cable event if we don't already have the cable (we might have created it)
			if(!find_cable_ptr(cable.get_input_URI(), cable.get_output_URI())) {
				ZstCable * cable_ptr = create_cable_ptr(cable);
                cable_ptr->set_activation_status(ZstSyncStatus::ACTIVATED);
				cable_arriving_events().enqueue(cable_ptr);
			}
			break;
		}
		case ZstMsgKind::DESTROY_CABLE:
		{
			const ZstCable & cable = msg->unpack_payload_serialisable<ZstCable>(i);
			ZstCable * cable_ptr = find_cable_ptr(cable.get_input_URI(), cable.get_output_URI());
			if (cable_ptr) {
				if (cable_ptr->is_activated()) {
					cable_ptr->enqueue_deactivation();
					this->cable_leaving_events().enqueue(cable_ptr);
				}
			}
			break;
		}
		case ZstMsgKind::CREATE_PLUG:
		{
			ZstPlug plug = msg->unpack_payload_serialisable<ZstPlug>(i);
			add_proxy_entity(plug);
			break;
		}
		case ZstMsgKind::CREATE_PERFORMER:
		{
			ZstPerformer performer = msg->unpack_payload_serialisable<ZstPerformer>(i);
			add_performer(performer);
			break;
		}
		case ZstMsgKind::CREATE_COMPONENT:
		{
			ZstComponent component = msg->unpack_payload_serialisable<ZstComponent>(i);
			add_proxy_entity(component);
			break;
		}
		case ZstMsgKind::CREATE_CONTAINER:
		{
			ZstContainer container = msg->unpack_payload_serialisable<ZstContainer>(i);
			add_proxy_entity(container);
			break;
		}
		case ZstMsgKind::DESTROY_ENTITY:
		{
			ZstURI entity_path = ZstURI(msg->payload_at(i).data(), msg->payload_at(i).size());
			
			//Only dispatch entity leaving events for non-local entities (for the moment)
			if (!path_is_local(entity_path)) {
				ZstEntityBase * entity = find_entity(entity_path);
                if(entity){
                    entity->enqueue_deactivation();
                    if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0 || strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
                        component_leaving_events().enqueue(entity);
                    } else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
                        plug_leaving_events().enqueue(entity);
                    } else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
                        performer_leaving_events().enqueue(entity);
                    }
                }
			}
			
			break;
		}
		case ZstMsgKind::REGISTER_COMPONENT_TEMPLATE:
		{
			throw std::logic_error("Handler for component type arriving mesages not implemented");
			break;
		}
		case ZstMsgKind::UNREGISTER_COMPONENT_TEMPLATE:
		{
			throw std::logic_error("Handler for component type leaving mesages not implemented");
			break;
		}
		default:
			ZstLog::net(LogLevel::notification, "Didn't understand message type of {}", payload_kind);
			throw std::logic_error("Didn't understand message type");
			break;
		}
	}
}

void ZstClient::connect_client_handler(const char * endpoint_ip, const char * output_plug) {
	ZstLog::net(LogLevel::notification, "Connecting to {}. My output endpoint is {}", endpoint_ip, m_graph_out_ip);

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, "%s", endpoint_ip);
	zsock_set_subscribe(m_graph_in, output_plug);
}

int ZstClient::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
	ZstClient * client = (ZstClient*)arg;
	chrono::time_point<chrono::system_clock> start = std::chrono::system_clock::now();
	ZstMessage * msg = client->msg_pool().get()->init_message(ZstMsgKind::CLIENT_HEARTBEAT);
	MessageFuture future = client->msg_pool().register_future(msg, true);

	try {
		future.then([&start, client](MessageFuture f) {
			int status = f.get();
			chrono::time_point<chrono::system_clock> end = chrono::system_clock::now();
			chrono::milliseconds delta = chrono::duration_cast<chrono::milliseconds>(end - start);
			ZstLog::net(LogLevel::notification, "Ping roundtrip {} ms", delta.count());
			client->m_ping = static_cast<long>(delta.count());
			return status;
		});
		client->send_to_stage(msg);
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Heartbeat timed out: {}", e.what());
	}

	return 0;
}


// ------------------
// Cable storage
// ------------------
ZstCable * ZstClient::create_cable_ptr(const ZstCable & cable)
{
	return create_cable_ptr(cable.get_input_URI(), cable.get_output_URI());
}

ZstCable * ZstClient::create_cable_ptr(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return create_cable_ptr(input->URI(), output->URI());
}

ZstCable * ZstClient::create_cable_ptr(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable_ptr = find_cable_ptr(input_path, output_path);
	if (cable_ptr) {
		return NULL;
	}
	
	//Create and store new cable
	cable_ptr = ZstCable::create(input_path, output_path);
	try {
		m_cables.insert(cable_ptr);
	}
	catch (std::exception e) {
		ZstLog::net(LogLevel::notification, "Couldn't insert cable. Reason:", e.what());
		ZstCable::destroy(cable_ptr);
		cable_ptr = NULL;
		return cable_ptr;
	}

	//Add cable to plugs
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(find_entity(input_path));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(find_entity(output_path));
	input_plug->add_cable(cable_ptr);
	output_plug->add_cable(cable_ptr);
	cable_ptr->set_input(input_plug);
	cable_ptr->set_output(output_plug);

	//Set network interactor
	cable_ptr->set_network_interactor(this);
	
	return cable_ptr;
}

ZstCable * ZstClient::find_cable_ptr(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstCable * cable = NULL;
	if (m_cables.size()) {
		auto search_cable = ZstCable(input_path, output_path);
		auto cable_ptr = m_cables.find(&search_cable);
		if (cable_ptr != m_cables.end()) {
			cable = *cable_ptr;
		}
	}
	return cable;
}

ZstCable * ZstClient::find_cable_ptr(ZstPlug * input, ZstPlug * output)
{
	if (!input || !output) {
		return NULL;
	}
	return find_cable_ptr(input->URI(), output->URI());
}


// --------
// Entities
// --------

ZstEntityBase * ZstClient::find_entity(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstPerformer * root = NULL;

	if (path_is_local(path)) {
		//Performer is local
		root = m_root;
	}
	else {
		//Performer is remote
		root = get_performer_by_URI(path);
	}

	if (!root)
		return result;

	if (root->URI() == path) {
		//Root is the entity we're searching for
		return root;
	}

	if (root) {
		//Find child entity in root
		result = root->find_child_by_URI(path);
	}

	return result;
}

ZstPlug * ZstClient::find_plug(const ZstURI & path)
{
	ZstComponent * plug_parent = dynamic_cast<ZstComponent*>(find_entity(path.parent()));
	return plug_parent->get_plug_by_URI(path);
}

void ZstClient::activate_entity(ZstEntityBase * entity, bool async)
{
	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
        ZstLog::net(LogLevel::notification, "No parent set for {}, adding to {}", entity->URI().path(), m_root->URI().path());
		m_root->add_child(entity);
	}

	//If this is not a local entity, we can't activate it
	if (!entity_is_local(*entity))
		return;

	//Register client to entity to allow it to send messages
	entity->set_network_interactor(this);
	entity->set_activating();
	
	//Build message
	ZstMessage * msg = msg_pool().get()->init_entity_message(entity);
	ZstURI entity_path = entity->URI();
    
	MessageFuture future = msg_pool().register_future(msg, true);
	if(async){
		activate_entity_async(entity, future);
		send_to_stage(msg);
	} else {
		send_to_stage(msg);
		activate_entity_sync(entity, future);
	}
}

void ZstClient::activate_entity_sync(ZstEntityBase * entity, MessageFuture & future)
{
	try {
		ZstMsgKind status = future.get();
		activate_entity_complete(status, entity);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Activate entity sync call timed out: {}", e.what());
	}
}

void ZstClient::activate_entity_async(ZstEntityBase * entity, MessageFuture & future)
{
	ZstURI entity_path(entity->URI());
	future.then([this, entity_path](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			status = f.get();
			ZstEntityBase * e = find_entity(entity_path);
			if (e)
				this->activate_entity_complete(status, e);
			else
				ZstLog::net(LogLevel::notification, "Entity {} went missing during activation!", entity_path.path());
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::notification, "Activate entity async call timed out: {}", e.what());
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::activate_entity_complete(ZstMsgKind status, ZstEntityBase * entity)
{
    if (status != ZstMsgKind::OK) {
        ZstLog::net(LogLevel::error, "Activate entity {} failed with status {}", entity->URI().path(), status);
        return;
    }
    
    if (status == ZstMsgKind::OK) {
        entity->enqueue_activation();
    }
    else if (status == ZstMsgKind::ERR_STAGE_ENTITY_ALREADY_EXISTS) {
        entity->set_error(ZstSyncError::ENTITY_ALREADY_EXISTS);
    }
    else if (status == ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND) {
        entity->set_error(ZstSyncError::PERFORMER_NOT_FOUND);
    }
    else if (status == ZstMsgKind::ERR_STAGE_ENTITY_NOT_FOUND) {
        entity->set_error(ZstSyncError::PARENT_NOT_FOUND);
    }
    
    ZstLog::net(LogLevel::notification, "Activate entity {} complete with status {}", entity->URI().path(), status);
}

void ZstClient::destroy_entity(ZstEntityBase * entity, bool async)
{
	if (!entity) {
		return;
	}

	//Set entity state as deactivating so we can't access it further
	entity->set_deactivating();

	//If the entity is local, let the stage know it's leaving
	if (entity_is_local(*entity)) {
        if(is_connected_to_stage()){
            ZstMessage * msg = msg_pool().get()->init_message(ZstMsgKind::DESTROY_ENTITY);
            msg->append_str(entity->URI().path(), entity->URI().full_size());
            ZstURI entity_path = entity->URI();
            
            MessageFuture future = msg_pool().register_future(msg, true);
			if (async) {
				destroy_entity_async(entity, future);
				send_to_stage(msg);
                
				entity->enqueue_deactivation();
				component_leaving_events().enqueue(entity);
			}
			else {
				send_to_stage(msg);
				destroy_entity_sync(entity, future);
			}
			
        } else {
            entity->enqueue_deactivation();
			component_leaving_events().enqueue(entity);
        }
	}
}

void ZstClient::destroy_entity_sync(ZstEntityBase * entity, MessageFuture & future)
{
	try {
		ZstMsgKind status = future.get();
		entity->enqueue_deactivation();
		component_leaving_events().enqueue(entity);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Destroy entity sync timed out: {}", e.what());
	}
}

void ZstClient::destroy_entity_async(ZstEntityBase * entity, MessageFuture & future)
{
	ZstURI entity_path(entity->URI());
	future.then([this, entity_path](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			status = f.get();
			if (status != ZstMsgKind::OK) {
				ZstLog::net(LogLevel::notification, "Destroy entity {} failed with status {}", entity_path.path(), status);
			}
			ZstLog::net(LogLevel::notification, "Destroy entity {} completed with status {}", entity_path.path(), status);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::notification, "Destroy entity async timed out: {}", e.what());
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::destroy_entity_complete(ZstMsgKind status, ZstEntityBase * entity)
{
	if (entity) {
		entity->set_destroyed();

        if(status != ZstMsgKind::OK){
            ZstLog::net(LogLevel::notification, "Destroy entity failed with status {}", status);
        }
        
		//Remove entity from parent
		if (entity->parent()) {
			ZstContainer * parent = dynamic_cast<ZstContainer*>(entity->parent());
			parent->remove_child(entity);
			entity->m_parent = NULL;
		}
        else {
			//Entity is a root performer. Remove from performer list
			m_clients.erase(entity->URI());
		}

		//Finally, add non-local entities to the reaper to destroy them at the correct time
		//TODO: Only destroying proxy entities at the moment. Local entities should be managed by the host application
		if(!entity_is_local(*entity))
			m_reaper.add(entity);
	}
}

bool ZstClient::entity_is_local(ZstEntityBase & entity)
{
	return path_is_local(entity.URI());
}

bool ZstClient::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

void ZstClient::add_proxy_entity(ZstEntityBase & entity) {
	
	//Don't need to activate local entities, they will auto-activate when the stage responds with an OK
	if (entity_is_local(entity)) {
		ZstLog::net(LogLevel::notification, "Received local entity {}. Ignoring", entity.URI().path());
		return;
	}
    
	ZstURI parent_URI = entity.URI().parent();
	if (parent_URI.size()) {
		ZstEntityBase * parent = find_entity(parent_URI);
			
		if (find_entity(entity.URI())) {
			ZstLog::net(LogLevel::error, "Can't create entity {}, it already exists", entity.URI().path());
			return;
		}

		ZstEntityBase * entity_proxy = NULL;
		
		//Create proxies and set parents
		if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
			entity_proxy = new ZstComponent(static_cast<ZstComponent&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			component_arriving_events().enqueue(entity_proxy);
		}
		else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
			entity_proxy = new ZstContainer(static_cast<ZstContainer&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			component_arriving_events().enqueue(entity_proxy);
		}
		else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
			ZstPlug * plug = new ZstPlug(static_cast<ZstPlug&>(entity));
			entity_proxy = plug;
			dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
			plug_arriving_events().enqueue(entity_proxy);
		}
		else {
			ZstLog::net(LogLevel::notification, "Can't create unknown proxy entity type {}", entity.entity_type());
		}
        
        ZstLog::net(LogLevel::notification, "Received proxy entity {}", entity_proxy->URI().path());
        
		//Forceably activate entity and dispatch events
		entity_proxy->set_network_interactor(this);
        entity_proxy->set_activation_status(ZstSyncStatus::ACTIVATED);
	}
	
}


// ----------
// Performers
// ----------

void ZstClient::add_performer(ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		ZstLog::net(LogLevel::debug, "Received self {} as performer. Ignoring", m_root->URI().path());
		return;
	}

	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
	assert(performer_proxy);
	ZstLog::net(LogLevel::notification, "Adding new performer {}", performer_proxy->URI().path());
    
    //Since this is a proxy entity, it should be activated immediately.
    performer_proxy->set_activation_status(ZstSyncStatus::ACTIVATED);

	if (performer.URI() != m_root->URI()) {
		m_clients[performer_proxy->URI()] = performer_proxy;
        performer_proxy->set_activation_status(ZstSyncStatus::ACTIVATED);
		performer_arriving_events().enqueue(performer_proxy);
	}
}

ZstPerformer * ZstClient::get_performer_by_URI(const ZstURI & uri) const
{
	ZstPerformer * result = NULL;
	ZstURI performer_URI = uri.first();

	auto entity_iter = m_clients.find(performer_URI);
	if (entity_iter != m_clients.end()) {
		result = entity_iter->second;
	}

	return result;
}

ZstPerformer * ZstClient::get_local_performer() const
{
	return m_root;
}

// -----
// Plugs
// -----

void ZstClient::destroy_plug(ZstPlug * plug, bool async)
{
    if (m_is_destroyed) {
        return;
    }

	plug->set_deactivating();
	
	if (entity_is_local(*plug)) {
		ZstMessage * msg = msg_pool().get()->init_message(ZstMsgKind::DESTROY_ENTITY);
		msg->append_str(plug->URI().path(), plug->URI().full_size());
		MessageFuture future = msg_pool().register_future(msg, true);
		if (async) {
			destroy_plug_async(plug, future);
			send_to_stage(msg);
		}
		else {
			send_to_stage(msg);
			destroy_plug_sync(plug, future);
		}
	}
}

void ZstClient::destroy_plug_sync(ZstPlug * plug, MessageFuture & future)
{
	try {
		ZstMsgKind status = future.get();
		destroy_plug_complete(status, plug);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Destroy plug timed out: {}", e.what());
	}
}

void ZstClient::destroy_plug_async(ZstPlug * plug, MessageFuture & future)
{
	try {
		future.then([this, plug](MessageFuture f) {
			ZstMsgKind status = f.get();
			this->destroy_plug_complete(status, plug);
			return status;
		});
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Destroy plug timed out: {}", e.what());
	}
}


void ZstClient::destroy_plug_complete(ZstMsgKind status, ZstPlug * plug)
{
	plug->set_destroyed();

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	//Finally, add to the reaper to destroy the plug at the correct time
	m_reaper.add(plug);
}

 // ------
 // Cables
 // ------
 
ZstCable * ZstClient::connect_cable(ZstPlug * input, ZstPlug * output, bool async)
{
	ZstCable * cable = NULL;

	if (!input || !output) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug missing.");
		return cable;
	}

	if (!input->is_activated() || !output->is_activated()) {
		ZstLog::net(LogLevel::notification, "Can't connect cable, plug is not activated.");
		return cable;
	}

	if (input->direction() != ZstPlugDirection::IN_JACK || output->direction() != ZstPlugDirection::OUT_JACK) {
		ZstLog::net(LogLevel::notification, "Cable order incorrect");
		return NULL;
	}
	
	cable = create_cable_ptr(input, output);
	if (!cable) {
		ZstLog::net(LogLevel::notification, "Couldn't create cable, already exists!");
		return NULL;
	}

	cable->set_activating();

	//If either of the cable plugs are a local entity, then the cable is local as well
	if (entity_is_local(*input) || entity_is_local(*output)) {
		cable->set_local();
	}

	//TODO: Even though we use a cable object when sending over the wire, it's up to us
	//to determine the correct input->output order - fix this using ZstInputPlug and 
	//ZstOutput plug as arguments
	ZstMessage * msg = msg_pool().get()->init_serialisable_message(ZstMsgKind::CREATE_CABLE, *cable);
    MessageFuture future = msg_pool().register_future(msg, true);
    
	if (async) {
		connect_cable_async(cable, future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		connect_cable_sync(cable, future);
	}

	//Create the cable early so we have something to return immediately
	return cable;
}

void ZstClient::connect_cable_sync(ZstCable * cable, MessageFuture & future)
{
	try {
		ZstMsgKind status = future.get();
		connect_cable_complete(status, cable);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Connect cable sync timed out: {}", e.what());
	}
}

void ZstClient::connect_cable_async(ZstCable * cable, MessageFuture & future)
{
	future.then([this, cable](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			status = f.get();
			this->connect_cable_complete(status, cable);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::notification, "Connect cable async timed out: {}", e.what());
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::connect_cable_complete(ZstMsgKind status, ZstCable * cable){
    if (status == ZstMsgKind::OK) {
        cable->enqueue_activation();
    } else {
        ZstLog::net(LogLevel::notification, "Cable connect for {}-{} failed with status {}", cable->get_input_URI().path(), cable->get_output_URI().path(), status);
    }
}

void ZstClient::destroy_cable(ZstCable * cable, bool async)
{
	//Need to set this cable as deactivating so the stage publish message doesn't clean it up too early
	cable->set_deactivating();
	ZstMessage * msg = msg_pool().get()->init_serialisable_message(ZstMsgKind::DESTROY_CABLE, *cable);
    
    MessageFuture future = msg_pool().register_future(msg, true);
	try {
		if (async) {
			destroy_cable_async(cable, future);
			send_to_stage(msg);
		}
		else {
			send_to_stage(msg);
			destroy_cable_sync(cable, future);
		}
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Destroy cable timed out: {}", e.what());
	}
}

void ZstClient::destroy_cable_sync(ZstCable * cable, MessageFuture & future)
{
    ZstMsgKind status(ZstMsgKind::EMPTY);
	try {
        status = future.get();
		cable->enqueue_deactivation();
		cable_leaving_events().enqueue(cable);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::notification, "Destroy cable sync timed out: ", e.what());
	}
}

void ZstClient::destroy_cable_async(ZstCable * cable, MessageFuture & future)
{
	future.then([this, cable](MessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		try {
			status = f.get();
			cable->enqueue_deactivation();
			this->cable_leaving_events().enqueue(cable);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::notification, "Destroy cable async timed out: {}", e.what());
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::destroy_cable_complete(ZstMsgKind status, ZstCable * cable)
{
    if(!cable)
        return;
	
    //Remove cable from local list so that other threads don't assume it still exists
    m_cables.erase(cable);
    
    if(status != ZstMsgKind::OK){
        ZstLog::net(LogLevel::notification, "Destroy cable failed with status {}", status);
    }
    ZstLog::net(LogLevel::notification, "Destroy cable completed with status {}", status);
    
    //Find the plugs and disconnect them seperately, in case they have already disappeared
    ZstPlug * input = dynamic_cast<ZstPlug*>(find_entity(cable->get_input_URI()));
    ZstPlug * output = dynamic_cast<ZstPlug*>(find_entity(cable->get_output_URI()));
    
    if (input)
        input->remove_cable(cable);
    
    if (output)
        output->remove_cable(cable);
    
    cable->set_input(NULL);
    cable->set_output(NULL);

	m_reaper.add(cable);
}

void ZstClient::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable_ptr(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}

ZstEventDispatcher & ZstClient::client_connected_events()
{
	return m_client_connected_event_manager;
}

ZstEventDispatcher & ZstClient::client_disconnected_events()
{
	return m_client_connected_event_manager;
}

ZstEventDispatcher & ZstClient::performer_arriving_events()
{
	return m_performer_arriving_event_manager;
}

ZstEventDispatcher & ZstClient::performer_leaving_events()
{
	return m_performer_leaving_event_manager;
}



// ------------------------
// Callback manager getters
// ------------------------

ZstEventDispatcher & ZstClient::component_arriving_events()
{
	return m_component_arriving_event_manager;
}

ZstEventDispatcher & ZstClient::component_leaving_events()
{
	return m_component_leaving_event_manager;
}

ZstEventDispatcher & ZstClient::component_type_arriving_events()
{
    return m_component_type_arriving_event_manager;
}

ZstEventDispatcher & ZstClient::component_type_leaving_events()
{
    return m_component_type_leaving_event_manager;
}

ZstEventDispatcher & ZstClient::plug_arriving_events()
{
	return m_plug_arriving_event_manager;
}

ZstEventDispatcher & ZstClient::plug_leaving_events()
{
	return m_plug_leaving_event_manager;
}

ZstEventDispatcher & ZstClient::cable_arriving_events()
{
	return m_cable_arriving_event_manager;
}

ZstEventDispatcher & ZstClient::cable_leaving_events()
{
	return m_cable_leaving_event_manager;
}

ZstEventDispatcher & ZstClient::compute_events()
{
	return m_compute_event_manager;
}

int ZstClient::graph_recv_tripmeter()
{
	return m_num_graph_recv_messages;
}

void ZstClient::reset_graph_recv_tripmeter()
{
	m_num_graph_recv_messages = 0;
}

int ZstClient::graph_send_tripmeter()
{
	return m_num_graph_send_messages;
}

void ZstClient::reset_graph_send_tripmeter()
{
	m_num_graph_send_messages = 0;
}

void ZstClient::enqueue_synchronisable_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_event_manager.enqueue(synchronisable);
}