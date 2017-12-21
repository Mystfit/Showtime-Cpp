#include <chrono>
#include <sstream>

#include "ZstClient.h"
#include "ZstCable.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstPlug.h"
#include "entities/ZstContainer.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPerformer.h"

//Core headers
#include "../core/ZstActor.h"
#include "../core/ZstMessages.h"
#include "../core/ZstValue.h"

using namespace std;

ZstClient::ZstClient() :
	m_root(NULL),
	m_num_graph_recv_messages(0),
    m_num_graph_send_messages(0),
	m_graph_out_ip("")
{
	m_component_arriving_event_manager = new ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*>();
	m_component_leaving_event_manager = new ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*>();
	m_component_type_arriving_event_manager = new ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*>();
	m_component_type_leaving_event_manager = new ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*>();
	m_cable_arriving_event_manager = new ZstCallbackQueue<ZstCableEvent, ZstCable*>();
	m_cable_leaving_event_manager = new ZstCallbackQueue<ZstCableEvent, ZstCable*>();
	m_plug_arriving_event_manager = new ZstCallbackQueue<ZstPlugEvent, ZstPlug*>();
	m_plug_leaving_event_manager = new ZstCallbackQueue<ZstPlugEvent, ZstPlug*>();
}

ZstClient::~ZstClient() {
	destroy();
	delete m_component_arriving_event_manager;
	delete m_component_leaving_event_manager;
	delete m_component_type_arriving_event_manager;
	delete m_component_type_leaving_event_manager;
	delete m_cable_arriving_event_manager;
	delete m_cable_leaving_event_manager;
	delete m_plug_arriving_event_manager;
	delete m_plug_leaving_event_manager;
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
	m_client_name = client_name;
	m_is_destroyed = false;

	m_component_leaving_event_manager->attach_post_event_callback(component_leaving_hook);
	m_component_type_leaving_event_manager->attach_post_event_callback(component_type_leaving_hook);
	m_cable_leaving_event_manager->attach_post_event_callback(cable_leaving_hook);
	m_plug_leaving_event_manager->attach_post_event_callback(plug_leaving_hook);

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
    cout << "ZST: Using external IP " << network_ip << endl;
    
	//Graph output socket
	std::stringstream addr;
	addr << "@tcp://" << network_ip.c_str() << ":*";
	m_graph_out_addr = addr.str();
    m_graph_out = zsock_new_pub(m_graph_out_addr.c_str());
	addr.str("");

	//Set graph socket as unbounded by HWM
	zsock_set_unbounded(m_graph_out);
	char * output_ip = zsock_last_endpoint(m_graph_out);
    m_graph_out_ip = std::string(output_ip);
	zstr_free(&output_ip);
    cout << "ZST: Client graph address: " << m_graph_out_ip << endl;
    
    if(m_graph_out)
        zsock_set_linger(m_graph_out, 0);
	
	//Connect client dealer socket to stage now
	addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
	m_stage_router_addr = addr.str();
	zsock_set_identity(m_stage_router, zuuid_str(m_startup_uuid));
	zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
	addr.str("");

	//Stage subscriber socket for update messages
	addr << "tcp://" << m_stage_addr << ":" << STAGE_PUB_PORT;
	m_stage_updates_addr = addr.str();
	cout << "ZST: Connecting to stage publisher " << m_stage_updates_addr << endl;
	zsock_connect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
	zsock_set_subscribe(m_stage_updates, "");
	addr.str("");

	//Create a root entity to hold our local entity hierarchy
	m_root = new ZstPerformer(m_client_name.c_str(), m_graph_out_addr.c_str());
	
    start();
}

void ZstClient::process_callbacks()
{
    //Run callbacks
    m_component_arriving_event_manager->process();
    m_component_leaving_event_manager->process();
    m_component_type_arriving_event_manager->process();
    m_component_type_leaving_event_manager->process();
    m_cable_arriving_event_manager->process();
    m_cable_leaving_event_manager->process();
    m_plug_arriving_event_manager->process();
    m_plug_leaving_event_manager->process();
    
    //Compute all entites
    while(m_compute_queue.size() > 0){
        ZstInputPlug * plug = m_compute_queue.pop();
        static_cast<ZstComponent*>(plug->parent())->compute(plug);
    }
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


// -------------
// Client init
// -------------

void ZstClient::register_client_to_stage(std::string stage_address) {
	cout << "ZST: Registering client" << endl;
	m_stage_addr = string(stage_address);

	//Build client addresses
	std::stringstream addr;

	//Build connect message
	zmsg_t * msg = ZstMessages::build_entity_message(ZstMessages::Kind::CLIENT_JOIN, m_root);
	send_to_stage(msg);
	
	//Check for stage acknowlegement of our connect request
	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type == ZstMessages::Kind::SIGNAL) {
		zframe_t * signal_type_frame = zmsg_pop(responseMsg);
		ZstMessages::Signal s = ZstMessages::unpack_signal(signal_type_frame);
		zframe_destroy(&signal_type_frame);

		if (s != ZstMessages::Signal::OK) {
			throw runtime_error("ZST: Stage performer registration responded with error -> Kind: " + std::to_string((int)message_type));
		}

		cout << "ZST: Successfully registered client to stage." << endl;
        
        //Set up heartbeat timer
        m_heartbeat_timer_id = attach_timer(s_heartbeat_timer, HEARTBEAT_DURATION, this);

		//TODO: Need a handshake with the stage before we mark connection as active
		m_connected_to_stage = true;
        
        //Ask the stage to send us a full snapshot
		signal_sync();
	}

	zmsg_destroy(&responseMsg);
}

void ZstClient::leave_stage()
{
	if (m_connected_to_stage) {
		cout << "ZST:Leaving stage" << endl;
		send_to_stage(ZstMessages::build_signal(ZstMessages::Signal::LEAVING));

		zsock_disconnect(m_stage_router, "%s", m_stage_router_addr.c_str());
		zsock_disconnect(m_stage_updates, "%s", m_stage_updates_addr.c_str());

		detach_timer(m_heartbeat_timer_id);

		m_connected_to_stage = false;
	}
}

bool ZstClient::is_connected_to_stage()
{
	return m_connected_to_stage;
}

void ZstClient::signal_sync()
{
	if (m_connected_to_stage) {
		cout << "ZST: Requesting stage snapshot" << endl;
		send_to_stage(ZstMessages::build_signal(ZstMessages::Signal::SYNC));
	}
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
			if (client->entity_is_local(receiving_plug)) {
				//TODO: Lock plug value when deserialising
				size_t offset = 0;
				receiving_plug->raw_value()->read((char*)zframe_data(payload), zframe_size(payload), offset);
				client->enqueue_compute(receiving_plug);
			}
		}
	}
	
	//Cleanup
	zframe_destroy(&payload);
    zmsg_destroy(&msg);
    free(msg);

	//Telemetrics
	client->m_num_graph_recv_messages++;
	return 0;
}

int ZstClient::s_handle_stage_update_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstClient *endpoint = (ZstClient*)arg;

	zmsg_t *msg = endpoint->receive_stage_update();
	endpoint->stage_update_handler(msg);
	zmsg_destroy(&msg);
	return 0;
}

int ZstClient::s_handle_stage_router(zloop_t * loop, zsock_t * socket, void * arg){
	ZstClient *endpoint = (ZstClient*)arg;

	//Receive routed message from stage
    zmsg_t *msg = endpoint->receive_from_stage();
    
	//Find message type from first frame
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
	if (message_type == ZstMessages::Kind::GRAPH_SNAPSHOT) {
		endpoint->stage_update_handler(msg);
	}
	else {
		
		switch (message_type) {
		case ZstMessages::Kind::SUBSCRIBE_TO_PERFORMER:
		{
			//Need endpoint ip only, cable will arrive via stage updates
			//Get message payload
			char * endpoint_ip = zmsg_popstr(msg);
			char * subscribed_output_plug = zmsg_popstr(msg);
			endpoint->connect_client_handler(endpoint_ip, subscribed_output_plug);
			zstr_free(&endpoint_ip);
			zstr_free(&subscribed_output_plug);
			break;
		}
		default:
			cout << "ZST: Performer dealer - Didn't understand message of type " << (char)message_type << endl;
			break;
		}
	}

	zmsg_destroy(&msg);
    return 0;
}

// ----------------------------------------------
// Unpacks batched update messages from the stage
// The frame structure for updates looks like this:
// | Kind | Payload | Kind | Payload | ... |
// ----------------------------------------------
void ZstClient::stage_update_handler(zmsg_t * msg)
{
	ZstMessages::Kind message_kind = ZstMessages::pop_message_kind_frame(msg);
	while (message_kind != ZstMessages::EMPTY)
	{
		zframe_t * msg_payload = zmsg_pop(msg);

		//Do something with payload
		switch (message_kind) {
			case ZstMessages::Kind::CREATE_CABLE:
			{
				ZstCable cable = ZstMessages::unpack_streamable<ZstCable>(msg_payload);
				create_cable_ptr(cable);
			}
			break;
			case ZstMessages::Kind::DESTROY_CABLE:
			{
				ZstCable cable = ZstMessages::unpack_streamable<ZstCable>(msg_payload);
				cable_leaving_events()->enqueue(find_cable_ptr(cable.get_input_URI(), cable.get_output_URI()));
			}
			break;
			case ZstMessages::Kind::CREATE_PLUG:
			{
				ZstPlug * plug = ZstMessages::unpack_entity<ZstPlug>(msg_payload);
				add_proxy_entity(plug);
				break;
			}
			case ZstMessages::Kind::DESTROY_PLUG:
			{
				ZstURI plug_path = ZstURI((char*)zframe_data(msg_payload));
				ZstPlug * plug = dynamic_cast<ZstPlug*>(find_plug(plug_path));
				plug_leaving_events()->enqueue(plug);
				break;
			}
			case ZstMessages::Kind::CREATE_PERFORMER:
			{
				ZstPerformer * performer = ZstMessages::unpack_entity<ZstPerformer>(msg_payload);
				add_performer(performer);
				break;
			}
			case ZstMessages::Kind::CREATE_COMPONENT: 
			{
				ZstComponent * component = ZstMessages::unpack_entity<ZstComponent>(msg_payload);
				add_proxy_entity(component);
			}
			case ZstMessages::Kind::CREATE_CONTAINER:
			{
				ZstContainer * container = ZstMessages::unpack_entity<ZstContainer>(msg_payload);
				add_proxy_entity(container);
			}
			case ZstMessages::Kind::DESTROY_ENTITY:
			{
				ZstURI entity_path = ZstURI((char*)zframe_data(msg_payload));
				ZstComponent * component = dynamic_cast<ZstComponent*>(find_entity(entity_path));
				component_leaving_events()->enqueue(component);
				break;
			}
			case ZstMessages::Kind::REGISTER_COMPONENT_TEMPLATE:
			{
				throw new std::exception("Handler for component type arriving mesages not implemented");
				break;
			}
			case ZstMessages::Kind::UNREGISTER_COMPONENT_TEMPLATE:
			{
				throw new std::exception("Handler for component type leaving mesages not implemented");
				break;
			}
			default:
				break;
			}

		//Cleanup payload
		zframe_destroy(&msg_payload);

		//Get next message in update
		message_kind = ZstMessages::pop_message_kind_frame(msg);
	}
}

void ZstClient::connect_client_handler(const char * endpoint_ip, const char * output_plug) {
	cout << "ZST: Connecting to " << endpoint_ip << ". My output endpoint is " << m_graph_out_ip << endl;

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, "%s", endpoint_ip);
	zsock_set_subscribe(m_graph_in, output_plug);
}

void ZstClient::create_entity_from_template_handler(const ZstURI & entity_template_address) {
    //TODO:Fill in entity template creation code
}

int ZstClient::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
	ZstClient * endpoint = (ZstClient*)arg;
	endpoint->send_to_stage(ZstMessages::build_signal(ZstMessages::Signal::HEARTBEAT));
	return 0;
}


// ------------------
// Item removal hooks
// ------------------
void ZstClient::component_leaving_hook(void * target)
{
	ZstClient::instance().destroy_entity(static_cast<ZstComponent*>(target));
}

void ZstClient::component_type_leaving_hook(void * target)
{
	throw new std::exception("Component type leaving hook not implemented");
}

void ZstClient::plug_leaving_hook(void * target)
{
	ZstPlug * plug = static_cast<ZstPlug*>(target);
	dynamic_cast<ZstComponent*>(plug->parent())->remove_plug(plug);
	ZstClient::instance().destroy_plug(plug);
}

void ZstClient::cable_leaving_hook(void * target)
{
	ZstClient::instance().remove_cable(static_cast<ZstCable*>(target));
}

// ------------------
// Cable storage
// ------------------
ZstCable * ZstClient::create_cable_ptr(ZstCable & cable)
{
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(find_entity(cable.get_input_URI()));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(find_entity(cable.get_output_URI()));

	ZstCable * cable_ptr = find_cable_ptr(input_plug->URI(), output_plug->URI());
	if (!cable_ptr) {
		cable_ptr = new ZstCable(input_plug, output_plug);
		input_plug->add_cable(cable_ptr);
		output_plug->add_cable(cable_ptr);
		cable_arriving_events()->enqueue(cable_ptr);
	}
	
	return cable_ptr;
}

void ZstClient::remove_cable(ZstCable * cable)
{
	cable->unplug();
	delete cable;
}

ZstCable * ZstClient::find_cable_ptr(const ZstURI & input_path, const ZstURI & output_path)
{
	ZstPlug * input_plug = dynamic_cast<ZstPlug*>(find_entity(input_path));
	ZstPlug * output_plug = dynamic_cast<ZstPlug*>(find_entity(output_path));
	ZstCable * cable_ptr = NULL;
	for (auto c : *output_plug) {
		if (c->get_input() == input_plug) {
			cable_ptr = c;
		}
	}

	return cable_ptr;
}


// ------------------------
// Entity type registration
// ------------------------

int ZstClient::register_component_type(ZstComponent * entity)
{
	throw new std::exception("Registering component types not implemented yet");
    return 0;
}

int ZstClient::unregister_component_type(ZstComponent * entity)
{
	throw new std::exception("Unregistering component types not implemented yet");
	return 0;
}

int ZstClient::run_component_template(ZstComponent * entity)
{
	throw new std::exception("Running component templates not implemented yet");
    return 0;
}


// --------
// Entities
// --------

ZstEntityBase * ZstClient::find_entity(const ZstURI & path)
{
	ZstEntityBase * result = NULL;
	ZstPerformer * root = NULL;

	if (path_is_local(path)) {
		root = m_root;
	}
	else {
		root = get_performer_by_URI(path);
	}

	if (root) {
		result = root->find_child_by_URI(path);
	}

	return result;
}

ZstPlug * ZstClient::find_plug(const ZstURI & path)
{
	ZstComponent * plug_parent = dynamic_cast<ZstComponent*>(find_entity(path.range(0, path.size() - 1)));
	return plug_parent->get_plug_by_URI(path);
}

int ZstClient::activate_entity(ZstEntityBase * entity)
{
	int result = 0;

	//If this is not a local entity, we can't activate it
	if (!entity_is_local(entity))
		return result;

	//If the entity doesn't have a parent, put it under the root container
	if (!entity->parent()) {
		m_root->add_child(m_root);
	}

	entity->register_graph_sender(this);
	
	std::stringstream buffer;
	entity->write(buffer);

	zmsg_t * msg = zmsg_new();
	zframe_t * kind_frame = ZstMessages::build_entity_kind_frame(entity);
	zmsg_append(msg, &kind_frame);
	zmsg_addmem(msg, buffer.str().c_str(), buffer.str().size());
	send_to_stage(msg);
	
    //TODO: Check if we can receive signals from the stage's router socket
	result = check_stage_response_ok();
	if (result) {
		//If this entity successfully activated on the stage, we can initialize it
		entity->set_activated();
		entity->init();
	}

	return result;
}

int ZstClient::destroy_entity(ZstEntityBase * entity)
{
	int result = 1;

	if (!entity || entity->is_destroyed()) {
		return result;
	}

	bool is_local = entity_is_local(entity);

	//Flag entity as destroyed so we can't remove it twice
	entity->set_destroyed();

	//If the entity is local, let the stage know it's leaving
	if (is_local && is_connected_to_stage()) {
		zmsg_t * msg = zmsg_new();
		zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::DESTROY_ENTITY);
		zmsg_append(msg, &kind_frame);
		zmsg_addstr(msg, entity->URI().path());
		send_to_stage(msg);
		result = check_stage_response_ok();
	}

	//Remove entity from parent
	if (entity->parent()) {
		ZstContainer * parent = dynamic_cast<ZstContainer*>(entity->parent());
		parent->remove_child(entity);
	}
	else {
		//Entity is a root performer. Remove from performer list
		m_clients.erase(entity->URI());
	}

	//If this entity is remote, we can clean it up
	if (is_local) {
		//TODO: Since this entity is local we can't wait for cables to be removed in the destructor
	}
	else {
		delete entity;
	}

	return result;
}

bool ZstClient::entity_is_local(ZstEntityBase * entity)
{
	return path_is_local(entity->URI());
}

bool ZstClient::path_is_local(const ZstURI & path) {
	return path.contains(m_root->URI());
}

void ZstClient::add_proxy_entity(ZstEntityBase * entity) {
	if (entity_is_local(entity)) {
		//TODO: Would be ideal to return here without having to delete an unused entity
		delete entity;
		return;
	}

	ZstPerformer * owning_performer = get_performer_by_URI(entity->URI());

	if (owning_performer) {
		ZstURI parent_URI = entity->URI().range(0, entity->URI().size() - 1);
		if (parent_URI.size()) {
			ZstEntityBase * parent = owning_performer->find_child_by_URI(parent_URI);

			if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
				dynamic_cast<ZstComponent*>(parent)->add_plug(dynamic_cast<ZstPlug*>(entity));
				component_arriving_events()->enqueue(entity);
			}
			else {
				dynamic_cast<ZstContainer*>(parent)->add_child(entity);
				component_arriving_events()->enqueue(entity);
			}
		}
	}
}


// ----------
// Performers
// ----------

std::unordered_map<ZstURI, ZstPerformer*> & ZstClient::performers()
{
	return m_clients;
}


void ZstClient::add_performer(ZstPerformer * performer)
{
	if (performer->URI() != m_root->URI()) {
		m_clients[performer->URI()] = performer;
	}
}

ZstPerformer * ZstClient::get_performer_by_URI(const ZstURI & uri) const
{
	ZstPerformer * result = NULL;
	ZstURI performer_URI = uri.range(0, 0);

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

int ZstClient::destroy_plug(ZstPlug * plug)
{
    int result = 0;
    if (m_is_destroyed || plug->is_destroyed()) {
        return result;
    }

	plug->set_destroyed();

	bool is_local = entity_is_local(plug);

	if (is_local) {
		zmsg_t * msg = zmsg_new();
		zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::DESTROY_ENTITY);
		zmsg_append(msg, &kind_frame);
		zmsg_addstr(msg, plug->URI().path());
		send_to_stage(msg);
		result = check_stage_response_ok();
	}

	ZstComponent * parent = dynamic_cast<ZstComponent*>(plug->parent());
	parent->remove_plug(plug);

	if (!is_local) {
		delete plug;
		plug = NULL;
		result = 1;
	
	}

    return result;
}

void ZstClient::enqueue_compute(ZstInputPlug * plug){
    m_compute_queue.push(plug);
}

 // ------
 // Cables
 // ------
 
ZstCable * ZstClient::connect_cable(ZstPlug * a, ZstPlug * b)
{
	 bool result = 0;
	 ZstCable cable;
	 ZstPlug * input_plug = NULL;
	 ZstPlug * output_plug = NULL;

	 if (a && b) {
		 if (a->direction() == ZstPlugDirection::OUT_JACK && b->direction() == ZstPlugDirection::IN_JACK) {
			 input_plug = b;
			 output_plug = a;
		 }
		 else if (a->direction() == ZstPlugDirection::IN_JACK && b->direction() == ZstPlugDirection::OUT_JACK) {
			 input_plug = a;
			 output_plug = b;
		 }
	 }

	cable = ZstCable(input_plug, output_plug);

	//Even though we use a cable object when sending over the wire, it's up to the stage
	//to determine the input->output order
	std::stringstream buffer;
	cable.write(buffer);

	zmsg_t * msg = zmsg_new();
	zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::CREATE_CABLE);
	zmsg_append(msg, &kind_frame);
	zmsg_addmem(msg, buffer.str().c_str(), buffer.str().size());
	send_to_stage(msg);

	ZstCable * cable_ptr = NULL;

	if (check_stage_response_ok()) {
		//Create the cable early so we have something to return immediately
		cable_ptr = create_cable_ptr(cable);
		result = 1;
	}

	return cable_ptr;
}

int ZstClient::destroy_cable(ZstCable * cable)
{
	std::stringstream buffer;
	cable->write(buffer);

	zmsg_t * msg = zmsg_new();
	zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::DESTROY_CABLE);
	zmsg_append(msg, &kind_frame);
	zmsg_addmem(msg, buffer.str().c_str(), buffer.str().size());
	send_to_stage(msg);
	return check_stage_response_ok();
}

void ZstClient::disconnect_plug(ZstPlug * plug)
{
	int result = 0;
	for (auto c : *plug) {
		destroy_cable(c);
	}
}

int ZstClient::disconnect_plugs(ZstPlug * input_plug, ZstPlug * output_plug)
{
	ZstCable * cable = find_cable_ptr(input_plug->URI(), output_plug->URI());
	return destroy_cable(cable);
}

// ---

int ZstClient::ping_stage()
{
	chrono::milliseconds delta = chrono::milliseconds(-1);
	chrono::time_point<chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	send_to_stage(ZstMessages::build_signal(ZstMessages::Signal::SYNC));

	if (check_stage_response_ok()) {
		end = chrono::system_clock::now();
		delta = chrono::duration_cast<chrono::milliseconds>(end - start);
		cout << "ZST: Client received heartbeat ping ack. Roundtrip was " << delta.count() << "ms" << endl;
	}
	return (int)delta.count();
}


// ------------------------
// Callback manager getters
// ------------------------

ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*> * ZstClient::component_arriving_events()
{
	return m_component_arriving_event_manager;
}

ZstCallbackQueue<ZstComponentEvent, ZstEntityBase*> * ZstClient::component_leaving_events()
{
	return m_component_leaving_event_manager;
}

ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * ZstClient::component_type_arriving_events()
{
    return m_component_type_arriving_event_manager;
}

ZstCallbackQueue<ZstComponentTypeEvent, ZstEntityBase*> * ZstClient::component_type_leaving_events()
{
    return m_component_type_leaving_event_manager;
}

ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * ZstClient::plug_arriving_events()
{
	return m_plug_arriving_event_manager;
}

ZstCallbackQueue<ZstPlugEvent, ZstPlug*> * ZstClient::plug_leaving_events()
{
	return m_plug_leaving_event_manager;
}

ZstCallbackQueue<ZstCableEvent, ZstCable*>* ZstClient::cable_arriving_events()
{
	return m_cable_arriving_event_manager;
}

ZstCallbackQueue<ZstCableEvent, ZstCable*>* ZstClient::cable_leaving_events()
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


// ------------
// Send/Receive
// ------------

void ZstClient::publish(ZstPlug * plug) {
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

void ZstClient::send_to_stage(zmsg_t * msg) {
	//Dealer socket doesn't add an empty frame to seperate identity chain and payload, so we handle it here
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg, &empty);
	zmsg_send(&msg, m_stage_router);
}

zmsg_t * ZstClient::receive_stage_update() {
	return zmsg_recv(m_stage_updates);
}

zmsg_t * ZstClient::receive_from_stage() {
	zmsg_t * msg = zmsg_recv(m_stage_router);

	//Pop blank seperator frame
	zframe_t * empty = zmsg_pop(msg);
    zframe_destroy(&empty);

	return msg;
}

ZstMessages::Signal ZstClient::check_stage_response_ok() {
	//Check for response from stage
	zmsg_t *responseMsg = receive_from_stage();

	//Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	//We're expecting a signal from the stage. Anything else is unexpected
	if (message_type != ZstMessages::Kind::SIGNAL) {
		throw runtime_error("ZST: Attempting to check stage signal, but we got a message other than signal!");
	}

	//Unpack signal
	zframe_t * signal_frame = zmsg_pop(responseMsg);
	ZstMessages::Signal s = ZstMessages::unpack_signal(signal_frame);
	if (s != ZstMessages::Signal::OK) {
		std::cout << "ZST: Stage responded with signal other than OK -> " << s << std::endl;
	}

	//Cleanup
	zframe_destroy(&signal_frame);
    zmsg_destroy(&responseMsg);
	return s;
}
