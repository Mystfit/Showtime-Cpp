#include <chrono>
#include <sstream>

#include "ZstClient.h"

using namespace std;

ZstClient::ZstClient() :
	m_root(NULL),
	m_num_graph_recv_messages(0),
    m_num_graph_send_messages(0),
	m_graph_out_ip(""),
	m_ping(-1)
{
	//Event queues
	m_synchronisable_event_manager = new ZstEventDispatcher();
	m_client_connected_event_manager = new ZstEventDispatcher();
	m_client_disconnected_event_manager = new ZstEventDispatcher();
	m_performer_arriving_event_manager = new ZstEventDispatcher();
	m_performer_leaving_event_manager = new ZstEventDispatcher();
	m_component_arriving_event_manager = new ZstEventDispatcher();
	m_component_leaving_event_manager = new ZstEventDispatcher();
	m_component_type_arriving_event_manager = new ZstEventDispatcher;
	m_component_type_leaving_event_manager = new ZstEventDispatcher();
	m_cable_arriving_event_manager = new ZstEventDispatcher();
	m_cable_leaving_event_manager = new ZstEventDispatcher();
	m_plug_arriving_event_manager = new ZstEventDispatcher();
	m_plug_leaving_event_manager = new ZstEventDispatcher();
	m_compute_event_manager = new ZstEventDispatcher();

	//Client events
	m_synchronisable_deferred_event = new ZstSynchronisableDeferredEvent();
	m_performer_leaving_hook = new ZstComponentLeavingEvent();
	m_component_leaving_hook = new ZstComponentLeavingEvent();
	m_cable_leaving_hook = new ZstCableLeavingEvent();
	m_plug_leaving_hook = new ZstPlugLeavingEvent();
	m_compute_event = new ZstComputeEvent();

	m_synchronisable_event_manager->attach_event_listener(m_synchronisable_deferred_event);
	m_performer_leaving_event_manager->attach_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager->attach_post_event_callback(m_component_leaving_hook);
	m_cable_leaving_event_manager->attach_post_event_callback(m_cable_leaving_hook);
	m_plug_leaving_event_manager->attach_post_event_callback(m_plug_leaving_hook);
	m_compute_event_manager->attach_event_listener(m_compute_event);

	//Message pools
	m_message_pool = new ZstMessagePool();
	m_message_pool->populate(MESSAGE_POOL_BLOCK);
}

ZstClient::~ZstClient() {
	destroy();

	m_synchronisable_event_manager->remove_event_listener(m_synchronisable_deferred_event);
	m_performer_leaving_event_manager->attach_post_event_callback(m_performer_leaving_hook);
	m_component_leaving_event_manager->remove_post_event_callback(m_component_leaving_hook);
	m_cable_leaving_event_manager->remove_post_event_callback(m_cable_leaving_hook);
	m_plug_leaving_event_manager->remove_post_event_callback(m_plug_leaving_hook);
	m_compute_event_manager->remove_post_event_callback(m_compute_event);

	delete m_synchronisable_event_manager;
	delete m_client_connected_event_manager;
	delete m_client_disconnected_event_manager;
	delete m_performer_arriving_event_manager;
	delete m_performer_leaving_event_manager;
	delete m_component_arriving_event_manager;
	delete m_component_leaving_event_manager;
	delete m_component_type_arriving_event_manager;
	delete m_component_type_leaving_event_manager;
	delete m_cable_arriving_event_manager;
	delete m_cable_leaving_event_manager;
	delete m_plug_arriving_event_manager;
	delete m_plug_leaving_event_manager;

	delete m_synchronisable_deferred_event;
	delete m_performer_leaving_hook;
	delete m_component_leaving_hook;
	delete m_cable_leaving_hook;
	delete m_plug_leaving_hook;
	delete m_compute_event;

	delete m_message_pool;
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

void ZstClient::init(const char * client_name)
{
	if (m_is_ending) {
		return;
	}
	zst_log_init();
	LOGGER->set_level(spdlog::level::debug);
	LOGGER->info("Starting Showtime v{}", SHOWTIME_VERSION);

	m_client_name = client_name;
	m_is_destroyed = false;
	
	ZstActor::init();
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
    LOGGER->debug("Using external IP {}", network_ip);
    
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
	LOGGER->debug("Client graph address: {}", m_graph_out_ip);
    
    if(m_graph_out)
        zsock_set_linger(m_graph_out, 0);
	
	//Connect client dealer socket to stage now
	addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
	m_stage_router_addr = addr.str();

	std::string identity = std::string(zuuid_str_canonical(m_startup_uuid));
	LOGGER->debug("Setting socket identity to {}. Length {}", identity, identity.size());

	zsock_set_identity(m_stage_router, identity.c_str());
	zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
	addr.str("");

	//Stage subscriber socket for update messages
	addr << "tcp://" << m_stage_addr << ":" << STAGE_PUB_PORT;
	m_stage_updates_addr = addr.str();

	LOGGER->debug("Connecting to stage publisher {}", m_stage_updates_addr);
	zsock_connect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
	zsock_set_subscribe(m_stage_updates, "");
	addr.str("");

	//Create a root entity to hold our local entity hierarchy
	m_root = new ZstPerformer(m_client_name.c_str(), m_graph_out_addr.c_str());
	m_root->set_network_interactor(this);
	
    start();
}

void ZstClient::process_callbacks()
{
	m_synchronisable_event_manager->process();
	m_client_connected_event_manager->process();
	m_client_disconnected_event_manager->process();
	m_performer_arriving_event_manager->process();
	m_performer_leaving_event_manager->process();
    m_component_arriving_event_manager->process();
    m_component_leaving_event_manager->process();
    m_component_type_arriving_event_manager->process();
    m_component_type_leaving_event_manager->process();
	m_cable_arriving_event_manager->process();
	m_cable_leaving_event_manager->process();
    m_plug_arriving_event_manager->process();
    m_plug_leaving_event_manager->process();
	m_compute_event_manager->process();
}

void ZstClient::clear_callbacks()
{
	m_synchronisable_event_manager->clear();
	m_client_connected_event_manager->clear();
	m_client_disconnected_event_manager->clear();
	m_cable_arriving_event_manager->clear();
	m_cable_leaving_event_manager->clear();
	m_performer_arriving_event_manager->clear();
	m_performer_leaving_event_manager->clear();
	m_component_arriving_event_manager->clear();
	m_component_leaving_event_manager->clear();
	m_component_type_arriving_event_manager->clear();
	m_component_type_leaving_event_manager->clear();
	m_plug_arriving_event_manager->clear();
	m_plug_leaving_event_manager->clear();
	m_compute_event_manager->clear();
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
	msg_pool()->release(msg);
}

ZstMessage * ZstClient::receive_stage_update() 
{
	ZstMessage * msg = NULL;
	zmsg_t * recv_msg = zmsg_recv(m_stage_updates);
	if (recv_msg) {
		msg = msg_pool()->get();
		msg->unpack(recv_msg);
	}
	return msg;
}

ZstMessage * ZstClient::receive_from_stage() {
	ZstMessage * msg = NULL;

	zmsg_t * recv_msg = zmsg_recv(m_stage_router);
	if (recv_msg) {
		msg = msg_pool()->get();

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

ZstMessagePool * ZstClient::msg_pool()
{
	return m_message_pool;
}


// -------------
// Client init
// -------------

void ZstClient::register_client_to_stage(std::string stage_address) {
	LOGGER->info("Connecting to stage {}", stage_address);

	m_stage_addr = string(stage_address);

	//Build client addresses
	std::stringstream addr;

	//Build connect message
	ZstMessage * msg = msg_pool()->get()->init_serialisable_message(ZstMessage::Kind::CLIENT_JOIN, *m_root);
	
	//Register complete action
	msg_pool()->register_future(msg).then([this](MessageFuture f){
		ZstMessage::Kind status = f.get();
		this->register_client_complete(status);
		return status;
	});
	
	send_to_stage(msg);
}

void ZstClient::register_client_complete(ZstMessage::Kind status)
{
	//If we didn't receive a OK signal, something went wrong
	if (status != ZstMessage::Kind::OK) {
		throw runtime_error("Stage performer registration responded with error -> Status: " + status);
	}

	LOGGER->info("Connection to server established");

	//Set up heartbeat timer
	m_heartbeat_timer_id = attach_timer(s_heartbeat_timer, HEARTBEAT_DURATION, this);

	//TODO: Need a handshake with the stage before we mark connection as active
	m_connected_to_stage = true;
	m_root->set_activated();
	client_connected_events()->enqueue(m_root);
	
	//Ask the stage to send us a full snapshot
	LOGGER->debug("Requesting stage snapshot");
	send_to_stage(msg_pool()->get()->init_message(ZstMessage::Kind::CLIENT_SYNC));
}


void ZstClient::leave_stage()
{
	if (m_connected_to_stage) {
		LOGGER->info("Leaving stage");
		
		//Close stage update socket so we don't receive any updates during shutdown
		zsock_disconnect(m_stage_updates, "%s", m_stage_updates_addr.c_str());

		//Purge callbacks
		clear_callbacks();

		//Notify stage that we are leaving
		ZstMessage * msg = msg_pool()->get();
		send_to_stage(msg_pool()->get()->init_message(ZstMessage::Kind::CLIENT_LEAVING));

		//Disconnect rest of sockets and timers
		zsock_disconnect(m_stage_router, "%s", m_stage_router_addr.c_str());
		detach_timer(m_heartbeat_timer_id);
		
		m_connected_to_stage = false;
	}
}

bool ZstClient::is_connected_to_stage()
{
	return m_connected_to_stage;
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
    
    //Get sender from msg
	//zframe_t * sender_frame = zmsg_pop(msg);
	char * sender_c = zmsg_popstr(msg);
	ZstURI sender = ZstURI(sender_c);
	zstr_free(&sender_c);
	
    //Get payload from msg
	zframe_t * payload = zmsg_pop(msg);

	//Find local proxy of the sneding plug
	ZstPlug * sending_plug = static_cast<ZstPlug*>(client->find_entity(sender));
	ZstInputPlug * receiving_plug = NULL;
	
	//Iterate over all connected cables from the sending plug
	for (auto cable : *sending_plug) {
		receiving_plug = dynamic_cast<ZstInputPlug*>(cable->get_input());
		if (receiving_plug) {
			if (client->entity_is_local(*receiving_plug)) {
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				receiving_plug->raw_value()->read((char*)zframe_data(payload), zframe_size(payload), offset);
				client->m_compute_event_manager->enqueue(receiving_plug);
			}
		}
	}
	
	//Cleanup
	zframe_destroy(&payload);
    zmsg_destroy(&msg);

	//Telemetrics
	client->m_num_graph_recv_messages++;
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

	//Process message promises
	if (client->msg_pool()->process_message_promise(msg) <= 0) {
		if (msg->kind() == ZstMessage::Kind::GRAPH_SNAPSHOT) {
			LOGGER->debug("Received graph snapshot");
			client->stage_update_handler(msg);
		}
		else {
			if (msg->kind() == ZstMessage::Kind::SUBSCRIBE_TO_PERFORMER) {
				client->connect_client_handler(msg->payload_at(0).data(), msg->payload_at(1).data());
			} else {
				LOGGER->error("Stage router sent unknown message {}", msg->kind());
			}
		}
	}
		
	client->msg_pool()->release(msg);
    return 0;
}

void ZstClient::stage_update_handler(ZstMessage * msg)
{
	ZstMessage::Kind payload_kind = msg->kind();
	if (payload_kind == ZstMessage::Kind::GRAPH_SNAPSHOT) {
		LOGGER->trace("Message is a graph snapshot, using payload kind");
	}
	else {
		LOGGER->trace("Message is a single stage update, using message kind");
	}

	for (size_t i = 0; i < msg->num_payloads(); ++i)
	{
		if (msg->kind() == ZstMessage::Kind::GRAPH_SNAPSHOT) {
			payload_kind = msg->payload_at(i).kind();
		}
		else {
			payload_kind = (msg->kind());
		}

		//Do something with payload
		switch (payload_kind) {
		case ZstMessage::Kind::CREATE_CABLE:
		{
			ZstCable cable = msg->unpack_payload_serialisable<ZstCable>(i);
			//Only dispatch cable event if we don't already have the cable (we might have created it)
			if(!find_cable_ptr(cable.get_input_URI(), cable.get_output_URI())) {
				ZstCable * cable_ptr = create_cable_ptr(cable);
				cable_ptr->set_activated();
				cable_arriving_events()->enqueue(cable_ptr);
			}
			break;
		}
		case ZstMessage::Kind::DESTROY_CABLE:
		{
			ZstCable cable = msg->unpack_payload_serialisable<ZstCable>(i);
			ZstCable * cable_ptr = find_cable_ptr(cable.get_input_URI(), cable.get_output_URI());
			if (cable_ptr) {
				//Find the plugs and disconnect them seperately, in case they have already disappeared
				ZstPlug * input = dynamic_cast<ZstPlug*>(find_entity(cable_ptr->get_input_URI()));
				ZstPlug * output = dynamic_cast<ZstPlug*>(find_entity(cable_ptr->get_output_URI()));

				if (input)
					input->remove_cable(cable_ptr);

				if (output)
					output->remove_cable(cable_ptr);

				cable_ptr->set_input(NULL);
				cable_ptr->set_output(NULL);
				cable_ptr->set_deactivated();
				cable_leaving_events()->enqueue(cable_ptr);
			}
			break;
		}
		case ZstMessage::Kind::CREATE_PLUG:
		{
			ZstPlug plug = msg->unpack_payload_serialisable<ZstPlug>(i);
			add_proxy_entity(plug);
			break;
		}
		case ZstMessage::Kind::DESTROY_PLUG:
		{
			ZstURI plug_path = ZstURI(msg->payload_at(i).data());
			ZstPlug * plug = dynamic_cast<ZstPlug*>(find_plug(plug_path));
			plug_leaving_events()->enqueue(plug);
			break;
		}
		case ZstMessage::Kind::CREATE_PERFORMER:
		{
			ZstPerformer performer = msg->unpack_payload_serialisable<ZstPerformer>(i);
			add_performer(performer);
			break;
		}
		case ZstMessage::Kind::CREATE_COMPONENT:
		{
			ZstComponent component = msg->unpack_payload_serialisable<ZstComponent>(i);
			add_proxy_entity(component);
			break;
		}
		case ZstMessage::Kind::CREATE_CONTAINER:
		{
			ZstContainer container = msg->unpack_payload_serialisable<ZstContainer>(i);
			add_proxy_entity(container);
			break;
		}
		case ZstMessage::Kind::DESTROY_ENTITY:
		{
			ZstURI entity_path = ZstURI(msg->payload_at(i).data(), msg->payload_at(i).size());
			
			//Only dispatch entity leaving events for non-local entities (for the moment)
			if (!path_is_local(entity_path)) {
				ZstEntityBase * entity = find_entity(entity_path);

				if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0 || strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
					component_leaving_events()->enqueue(entity);
				} else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
					plug_leaving_events()->enqueue(entity);
				} else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
					performer_leaving_events()->enqueue(entity);
				}
			}
			
			break;
		}
		case ZstMessage::Kind::REGISTER_COMPONENT_TEMPLATE:
		{
			throw new std::exception("Handler for component type arriving mesages not implemented");
			break;
		}
		case ZstMessage::Kind::UNREGISTER_COMPONENT_TEMPLATE:
		{
			throw new std::exception("Handler for component type leaving mesages not implemented");
			break;
		}
		default:
			LOGGER->error("Didn't understand message type of {}", payload_kind);
			throw new std::exception("");
			break;
		}
	}
}

void ZstClient::connect_client_handler(const char * endpoint_ip, const char * output_plug) {
	LOGGER->debug("Connecting to {}. My output endpoint is {}", endpoint_ip, m_graph_out_ip);

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, "%s", endpoint_ip);
	zsock_set_subscribe(m_graph_in, output_plug);
}

void ZstClient::create_entity_from_template_handler(const ZstURI & entity_template_address) {
    //TODO:Fill in entity template creation code
}

int ZstClient::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
	ZstClient * client = (ZstClient*)arg;
	
	chrono::time_point<chrono::system_clock> start = std::chrono::system_clock::now();

	ZstMessage * msg = client->msg_pool()->get()->init_message(ZstMessage::Kind::CLIENT_HEARTBEAT);
	client->msg_pool()->register_future(msg).then([&start, client](MessageFuture f) {
		int status = f.get();
		chrono::time_point<chrono::system_clock> end = chrono::system_clock::now();
		chrono::milliseconds delta = chrono::duration_cast<chrono::milliseconds>(end - start);
		LOGGER->debug("Ping roundtrip {} ms", delta.count());
		client->m_ping = static_cast<long>(delta.count());
		return status;
	});
	client->send_to_stage(msg);

	return 0;
}


// ------------------
// Cable storage
// ------------------
ZstCable * ZstClient::create_cable_ptr(ZstCable & cable)
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
	cable_ptr = new ZstCable(input_path, output_path);
	try {
		m_cables.insert(cable_ptr);
	}
	catch (std::exception e) {
		LOGGER->error("Couldn't insert cable. Reason:", e.what());
		delete cable_ptr;
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

void ZstClient::activate_entity(ZstEntityBase * entity)
{
	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
		m_root->add_child(entity);
	}

	//If this is not a local entity, we can't activate it
	if (!entity_is_local(*entity))
		return;

	//Register client to entity to allow it to send messages
	entity->set_network_interactor(this);
	entity->set_activating();
	
	//Build message
	ZstMessage * msg = msg_pool()->get()->init_entity_message(entity);
	ZstURI entity_path = entity->URI();
	msg_pool()->register_future(msg).then([this, entity_path](MessageFuture f) {
		ZstMessage::Kind status = f.get();
		if (status != ZstMessage::Kind::OK) {
			LOGGER->warn("Activate entity {} failed with status {}", entity_path.path(), status);
			return status;
		}

		//In case the entity has disappeared during activation (for whatever reason) make sure it exists first
		ZstEntityBase * e = find_entity(entity_path);
		if (e) {

			if (status == ZstMessage::Kind::OK) {
				e->set_activated();
			}
			else if (status == ZstMessage::Kind::ERR_STAGE_ENTITY_ALREADY_EXISTS) {
				e->set_error(ZstSyncError::ENTITY_ALREADY_EXISTS);
			} 
			else if (status == ZstMessage::Kind::ERR_STAGE_ENTITY_NOT_FOUND) {
				e->set_error(ZstSyncError::PERFORMER_NOT_FOUND);
			}
			else if (status == ZstMessage::Kind::ERR_STAGE_ENTITY_NOT_FOUND) {
				e->set_error(ZstSyncError::PARENT_NOT_FOUND);
			}

			LOGGER->debug("Activate entity {} complete with status {}", e->URI().path(), status);
		}
		
		return status;
	});
	send_to_stage(msg);
}

void ZstClient::destroy_entity(ZstEntityBase * entity)
{
	if (!entity || entity->is_destroyed()) {
		return;
	}

	entity->set_deactivating();

	//If the entity is local, let the stage know it's leaving
	if (entity_is_local(*entity) && is_connected_to_stage()) {
		ZstMessage * msg = msg_pool()->get()->init_message(ZstMessage::Kind::DESTROY_ENTITY);
		msg->append_str(entity->URI().path(), entity->URI().full_size());
		ZstURI entity_path = entity->URI();
		msg_pool()->register_future(msg).then([this, entity_path](MessageFuture f) {
			ZstMessage::Kind status = f.get();
			if (status != ZstMessage::Kind::OK) {
				LOGGER->error("Destroy entity {} failed with status {}", entity_path.path(), status);
				return status;
			}
			LOGGER->debug("Destroy entity {} completed with status {}", entity_path.path(), status);
			return status;
		});
		send_to_stage(msg);

		//Since we own this entity, we can start to clean it up immediately
		entity->set_deactivated();
		destroy_entity_complete(entity);
	}
}

void ZstClient::destroy_entity_complete(ZstEntityBase * entity)
{
	if (entity) {
		if (entity->is_destroyed())
			return;

		//Flag entity as destroyed so we can't remove it twice
		entity->set_destroyed();

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

		//Make sure to remove all cables from this entity
		/*auto cable_bundle = entity->acquire_cable_bundle();
		for (int i = 0; i < cable_bundle->size(); ++i) {
			ZstCable * cable = cable_bundle->cable_at(i);
			if (cable->is_activated()) {
				cable->set_deactivated();
				cable_leaving_events()->enqueue(cable_bundle->cable_at(i));
			}
		}
		entity->release_cable_bundle(cable_bundle);*/

		//If this entity is remote, we can clean it up
		if (!entity_is_local(*entity)) {
			delete entity;
		}
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
		LOGGER->info("Received local entity {}. Ignoring", entity.URI().path());
		return;
	}

	//Copy streamable so we have a local ptr for the entity
	ZstEntityBase * entity_proxy = NULL;

	ZstURI parent_URI = entity.URI().parent();
	if (parent_URI.size()) {
		ZstEntityBase * parent = find_entity(parent_URI);
			
		if (find_entity(entity.URI())) {
			LOGGER->warn("Can't create entity {}, it already exists", entity.URI().path());
			return;
		}

		ZstEntityBase * entity_proxy = NULL;
		ZstEventDispatcher * dispatcher = NULL;

		//Create proxies and set parents
		if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
			entity_proxy = new ZstComponent(static_cast<ZstComponent&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			dispatcher = component_arriving_events();
		}
		else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
			entity_proxy = new ZstContainer(static_cast<ZstContainer&>(entity));
			dynamic_cast<ZstContainer*>(parent)->add_child(entity_proxy);
			dispatcher = component_arriving_events();
		}
		else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
			ZstPlug * plug = new ZstPlug(static_cast<ZstPlug&>(entity));
			entity_proxy = plug;
			dynamic_cast<ZstComponent*>(parent)->add_plug(plug);
			dispatcher = plug_arriving_events();
		}
		else {
			LOGGER->error("Can't create unknown proxy entity type {}", entity.entity_type());
		}

		//Activate entity and dispatch events
		entity_proxy->set_network_interactor(this);
		entity_proxy->set_activated();
		dispatcher->enqueue(entity_proxy);
	}
	
}


// ----------
// Performers
// ----------

void ZstClient::add_performer(ZstPerformer & performer)
{
	if (performer.URI() == m_root->URI()) {
		LOGGER->warn("Received self {} as performer. Ignoring", m_root->URI().path());
		return;
	}

	//Copy streamable so we have a local ptr for the performer
	ZstPerformer * performer_proxy = new ZstPerformer(performer);
	assert(performer_proxy);
	LOGGER->debug("Adding new performer {}", performer_proxy->URI().path());

	if (performer.URI() != m_root->URI()) {
		m_clients[performer_proxy->URI()] = performer_proxy;
		performer_proxy->set_activated();
		performer_arriving_events()->enqueue(performer_proxy);
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

void ZstClient::destroy_plug(ZstPlug * plug)
{
    if (m_is_destroyed || plug->is_destroyed()) {
        return;
    }

	plug->set_destroyed();
	plug->set_deactivating();

	bool is_local = entity_is_local(*plug);

	if (is_local) {
		ZstMessage * msg = msg_pool()->get()->init_message(ZstMessage::Kind::DESTROY_ENTITY);
		msg->append_str(plug->URI().path(), plug->URI().full_size());
		msg_pool()->register_future(msg).then([this](MessageFuture f) {
			int status = f.get();
			this->destroy_plug_complete(status);
			return status;
		});
	}

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	if (!is_local) {
		delete plug;
		plug = NULL;
	}
}

void ZstClient::destroy_plug_complete(int status)
{	
}

 // ------
 // Cables
 // ------
 
ZstCable * ZstClient::connect_cable(ZstPlug * input, ZstPlug * output)
{
	ZstCable * cable = NULL;

	if (!input || !output) {
		LOGGER->error("Can't connect cable, plug missing.");
		return cable;
	}

	if (!input->is_activated() || !output->is_activated()) {
		LOGGER->error("Can't connect cable, plug is not activated.");
		return cable;
	}

	if (input->direction() != ZstPlugDirection::IN_JACK || output->direction() != ZstPlugDirection::OUT_JACK) {
		LOGGER->error("Cable order incorrect");
		return NULL;
	}
	
	cable = create_cable_ptr(input, output);
	if (!cable) {
		LOGGER->error("Couldn't create cable, already exists!");
		return NULL;
	}

	cable->set_activating();

	//If either of the cable plugs are a local entity, then the cable is local as well
	if (entity_is_local(*input) || entity_is_local(*output)) {
		cable->set_local();
	}

	//Even though we use a cable object when sending over the wire, it's up to the stage
	//to determine the input->output order

	ZstMessage * msg = msg_pool()->get()->init_serialisable_message(ZstMessage::Kind::CREATE_CABLE, *cable);
	msg_pool()->register_future(msg).then([this, cable](MessageFuture f) {
		int status = f.get();
		if (status == ZstMessage::Kind::OK) {
			cable->set_activated();
		}
		return status;
	});
	send_to_stage(msg);

	//Create the cable early so we have something to return immediately
	return cable;
}

void ZstClient::destroy_cable(ZstCable * cable)
{
	cable->set_deactivating();
	ZstMessage * msg = msg_pool()->get()->init_serialisable_message(ZstMessage::Kind::DESTROY_CABLE, *cable);
	msg_pool()->register_future(msg).then([this, cable](MessageFuture f) {
		int status = f.get();
		LOGGER->debug("Destroy cable completed with status {}", status);
		cable->set_deactivated();
		return status;
	});
	send_to_stage(msg);
}

void ZstClient::disconnect_plug(ZstPlug * plug)
{
	int result = 0;
	for (auto c : *plug) {
		destroy_cable(c);
	}
}

void ZstClient::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable_ptr(input_plug->URI(), output_plug->URI());
	destroy_cable(cable);
}

ZstEventDispatcher * ZstClient::client_connected_events()
{
	return m_client_connected_event_manager;
}

ZstEventDispatcher * ZstClient::client_disconnected_events()
{
	return m_client_connected_event_manager;
}

ZstEventDispatcher * ZstClient::performer_arriving_events()
{
	return m_performer_arriving_event_manager;
}

ZstEventDispatcher * ZstClient::performer_leaving_events()
{
	return m_performer_leaving_event_manager;
}



// ------------------------
// Callback manager getters
// ------------------------

ZstEventDispatcher * ZstClient::component_arriving_events()
{
	return m_component_arriving_event_manager;
}

ZstEventDispatcher * ZstClient::component_leaving_events()
{
	return m_component_leaving_event_manager;
}

ZstEventDispatcher * ZstClient::component_type_arriving_events()
{
    return m_component_type_arriving_event_manager;
}

ZstEventDispatcher * ZstClient::component_type_leaving_events()
{
    return m_component_type_leaving_event_manager;
}

ZstEventDispatcher * ZstClient::plug_arriving_events()
{
	return m_plug_arriving_event_manager;
}

ZstEventDispatcher * ZstClient::plug_leaving_events()
{
	return m_plug_leaving_event_manager;
}

ZstEventDispatcher * ZstClient::cable_arriving_events()
{
	return m_cable_arriving_event_manager;
}

ZstEventDispatcher * ZstClient::cable_leaving_events()
{
	return m_cable_leaving_event_manager;
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
	m_synchronisable_event_manager->enqueue(synchronisable);
}
