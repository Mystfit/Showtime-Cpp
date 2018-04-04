#include <chrono>
#include <sstream>

#include "ZstClient.h"

using namespace std;

ZstClient::ZstClient() :
    m_ping(-1),
    m_is_ending(false),
    m_is_destroyed(false),
    m_init_completed(false),
    m_connected_to_stage(false)
{
	add_event_queue(m_client_connected_event_manager);
	add_event_queue(m_client_disconnected_event_manager);
	add_event_queue(m_compute_event_manager);
	add_event_queue(m_synchronisable_event_manager);

	m_compute_event = new ZstComputeEvent();
	m_compute_event_manager.attach_event_listener(m_compute_event);

	m_synchronisable_deferred_event = new ZstSynchronisableDeferredEvent();
	m_synchronisable_event_manager.attach_event_listener(m_synchronisable_deferred_event);
	
	//Client events
	
	m_cable_leaving_hook = new ZstCableLeavingEvent();

	
	m_performer_leaving_event_manager.attach_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager.attach_post_event_callback(m_component_leaving_hook);
	m_cable_leaving_event_manager.attach_post_event_callback(m_cable_leaving_hook);
	m_plug_leaving_event_manager.attach_post_event_callback(m_plug_leaving_hook);
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
	if(is_connected_to_stage())
		leave_stage(true);
    
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


void ZstClient::start() {
	ZstActor::start();
}

void ZstClient::stop() {
	ZstActor::stop();
}


// ------------
// Send/Receive
// ------------



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
	
	MessageFuture future = msg_pool().register_response_message(msg, true);
	if(async){
		register_client_to_stage_async(future);
		send_to_stage(msg);
	}
	else {
		send_to_stage(msg);
		join_stage(future);
	}
}

void ZstClient::join_stage(MessageFuture & future)
{
	ZstMsgKind status(ZstMsgKind::EMPTY);
	try {
		status = future.get();
		join_stage_complete(status);
		synchronise_graph(false);
		process_callbacks();
	}
	catch (const ZstTimeoutException & e) {
		ZstLog::net(LogLevel::error, "Stage sync join timed out - {}", e.what());
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
			this->join_stage_complete(status);
			this->synchronise_graph(true);
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::error, "Stage async join timed out - {}", e.what());
			leave_stage_complete();
			status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}
		return status;
	});
}

void ZstClient::join_stage_complete(ZstMsgKind status)
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
	MessageFuture future = msg_pool().register_response_message(msg, true);
	
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
		MessageFuture future = msg_pool().register_response_message(msg, true);
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


int ZstClient::graph_message_handler(zmsg_t * msg) {
	//Get sender from msg
	char * sender_c = zmsg_popstr(msg);
	ZstURI sender(sender_c);
	zstr_free(&sender_c);

	//Get payload from msg
	zframe_t * payload = zmsg_pop(msg);

	//Find local proxy of the sneding plug
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
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				receiving_plug->raw_value()->read((char*)zframe_data(payload), zframe_size(payload), offset);
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
	MessageFuture future = client->msg_pool().register_response_message(msg, true);

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



// --------
// Entities
// --------




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




// ----------
// Performers
// ----------



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


 // ------
 // Cables
 // ------
 

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



ZstEventQueue & ZstClient::client_connected_events()
{
	return m_client_connected_event_manager;
}

ZstEventQueue & ZstClient::client_disconnected_events()
{
	return m_client_connected_event_manager;
}

ZstEventQueue & ZstClient::compute_events()
{
	return m_compute_event_manager;
}


void ZstClient::enqueue_synchronisable_event(ZstSynchronisable * synchronisable)
{
	m_synchronisable_event_manager.enqueue(synchronisable);
}