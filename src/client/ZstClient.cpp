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
	static ZstClient endpoint_singleton;
	return endpoint_singleton;
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
    cout << "ZST: Endpoint graph address: " << m_graph_out_ip << endl;
    
    if(m_graph_out)
        zsock_set_linger(m_graph_out, 0);

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
// Endpoint init
// -------------

void ZstClient::register_client_to_stage(std::string stage_address) {
	cout << "ZST: Registering endpoint" << endl;
	m_stage_addr = string(stage_address);

	//Build endpoint addresses
	std::stringstream addr;
	
	//Package our root container for the stage
	std::stringstream entity_buffer;
	m_root->write(entity_buffer);

	//Build connect message
	zmsg_t * msg = zmsg_new();
	zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::CLIENT_JOIN);
	zmsg_append(msg, &kind_frame);						//Pack kind
	zmsg_addstr(msg, entity_buffer.str().c_str());		//Pack our local performer
	send_to_stage(msg);

	//Check for stage acknowlegement of our connect request
	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type == ZstMessages::Signal::OK) {
		char * assigned_uuid = zmsg_popstr(responseMsg);
		m_assigned_uuid = std::string(assigned_uuid);
		zstr_free(&assigned_uuid);
		cout << "ZST: Successfully registered endpoint to stage. UUID is " << m_assigned_uuid << endl;

		//Connect client dealer socket to stage now that it's been registered successfully
		addr << "tcp://" << m_stage_addr << ":" << STAGE_ROUTER_PORT;
		m_stage_router_addr = addr.str();
		zsock_set_identity(m_stage_router, m_assigned_uuid.c_str());
		zsock_connect(m_stage_router, "%s", m_stage_router_addr.c_str());
		addr.str("");
		
		//Stage subscriber socket for update messages
		addr << "tcp://" << stage_address << ":" << STAGE_PUB_PORT;
		m_stage_updates_addr = addr.str();
		cout << "ZST: Connecting to stage publisher " << m_stage_updates_addr << endl;
		zsock_connect(m_stage_updates, "%s", m_stage_updates_addr.c_str());
		zsock_set_subscribe(m_stage_updates, "");
		addr.str("");
        
        //Set up heartbeat timer
        m_heartbeat_timer_id = attach_timer(s_heartbeat_timer, HEARTBEAT_DURATION, this);

		//TODO: Need a handshake with the stage before we mark connection as active
		m_connected_to_stage = true;
        
        //Ask the stage to send us a full snapshot
		signal_sync();
	}
	else {
		throw runtime_error("ZST: Stage performer registration responded with error -> Kind: " + std::to_string((int)message_type));
	}
	zmsg_destroy(&responseMsg);
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
	zframe_t * sender_frame = zmsg_pop(msg);
	ZstURI sender = ZstMessages::unpack_streamable<ZstURI>(sender_frame);
	zframe_destroy(&sender_frame);
	
    //Get payload from msg
	zframe_t * payload = zmsg_pop(msg);

	//Find the local cable matching this message
	for (ZstCable * cable : client->m_cables) {
		if (ZstURI::equal(cable->get_output(), sender)) {
			//Deserialise value into target plug
			ZstInputPlug * plug = (ZstInputPlug*)client->find_plug(cable->get_input());
			size_t offset = 0;
			if (plug) {
				//TODO: Lock plug value when deserialising
				plug->raw_value()->read((char*)zframe_data(payload), zframe_size(payload), offset);
				client->enqueue_compute(plug);
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
	int num_messages = static_cast<int>(zmsg_size(msg)) / 2;
	if (num_messages % 2 != 0) {
		throw new std::exception("Number of received stage messages not even (Kind/Payload)");
	}
	for (int i = 0; i < num_messages; ++i) {
		ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
		zframe_t * msg_payload = zmsg_pop(msg);

		//Do something with payload
		switch (message_type) {
			case ZstMessages::Kind::CREATE_CABLE:
			{
				ZstCable * cable = ZstMessages::unpack_entity<ZstCable>(msg_payload);
				add_cable(cable);
			}
			break;
			case ZstMessages::Kind::DESTROY_CABLE:
			{
				ZstCable * cable = ZstMessages::unpack_entity<ZstCable>(msg_payload);
				ZstCable * local_cable = get_cable_by_URI(cable->get_input(), cable->get_output());
				cable_leaving_events()->enqueue(local_cable);
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
				ZstURI plug_path = ZstMessages::unpack_streamable<ZstURI>(msg_payload);
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
				ZstURI entity_path = ZstMessages::unpack_streamable<ZstURI>(msg_payload);
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

bool ZstClient::is_connected_to_stage()
{
	return m_connected_to_stage;
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
	zmsg_addstr(msg, buffer.str().c_str());
	send_to_stage(msg);
	
    //TODO: Check if we can receive signals from the stage's router socket
	result = check_stage_response_ok();
	if (result) {
		entity->set_activated();
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
		std::stringstream buffer;
		ZstURI(entity->URI()).write(buffer);

		zmsg_t * msg = zmsg_new();
		zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::DESTROY_ENTITY);
		zmsg_append(msg, &kind_frame);
		zmsg_addstr(msg, buffer.str().c_str());
		send_to_stage(msg);
		result = check_stage_response_ok();
	}

	//Remove entity from parent
	if (entity->parent()) {
		ZstContainer * parent = dynamic_cast<ZstContainer*>(entity->parent());
		parent->remove_child(entity);
	}
	else {
		//Must be a root container. Remove from root list
		m_clients.erase(entity->URI());
	}

	if (!is_local) {
		//Signal leaving callbacks
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

ZstContainer * ZstClient::get_local_performer() const
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
		std::stringstream buffer;
		ZstURI(plug->URI()).write(buffer);

		zmsg_t * msg = zmsg_new();
		zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::DESTROY_ENTITY);
		zmsg_append(msg, &kind_frame);
		zmsg_addstr(msg, buffer.str().c_str());
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
 
 int ZstClient::connect_cable(ZstPlug * a, ZstPlug * b)
{
	 bool result = 0;
	 ZstCable * cable = NULL;

	 if (a && b) {
		 if (a->direction() == ZstPlugDirection::OUT_JACK && b->direction() == ZstPlugDirection::IN_JACK) {
			 cable = new ZstCable(b, a);
		 }
		 else if (a->direction() == ZstPlugDirection::IN_JACK && b->direction() == ZstPlugDirection::OUT_JACK) {
			 cable = new ZstCable(a, b);
		 }
	 }

	//Even though we use a cable object when sending over the wire, it's up to the stage
	//to determine the input->output order
	std::stringstream buffer;
	cable->write(buffer);

	zmsg_t * msg = zmsg_new();
	zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::CREATE_CABLE);
	zmsg_append(msg, &kind_frame);
	zmsg_addstr(msg, buffer.str().c_str());
	send_to_stage(msg);

	if (check_stage_response_ok()) {
		m_cables.push_back(cable);
		result = 1;
	}

	return result;
}

int ZstClient::destroy_cable(ZstCable * cable)
{
	std::stringstream buffer;
	cable->write(buffer);

	zmsg_t * msg = zmsg_new();
	zframe_t * kind_frame = ZstMessages::build_message_kind_frame(ZstMessages::Kind::DESTROY_CABLE);
	zmsg_append(msg, &kind_frame);
	zmsg_addstr(msg, buffer.str().c_str());
	send_to_stage(msg);
	return check_stage_response_ok();
}


vector<ZstCable*> ZstClient::get_cables_by_URI(const ZstURI & uri) {

	vector<ZstCable*> cables;
	auto it = find_if(m_cables.begin(), m_cables.end(), [&uri](ZstCable* current) {
		return current->is_attached(uri);
	});
    
	for (it; it != m_cables.end(); ++it) {
		cables.push_back((*it));
	}

	return cables;
}

ZstCable * ZstClient::get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB) {

	auto it = find_if(m_cables.begin(), m_cables.end(), [&uriA, &uriB](ZstCable * current) {
		return current->is_attached(uriA, uriB);
	});

	if (it != m_cables.end()) {
		return (*it);
	}
	return NULL;
}

void ZstClient::add_cable(ZstCable * cable)
{
	for (auto c : m_cables) {
		if (*(c) == *(cable)) {
			std::cout << "ZST: Can't add cable, already exists" << std::endl;
			return;
		}
	}
	m_cables.push_back(cable);
	cable_arriving_events()->enqueue(cable);
}

void ZstClient::remove_cable(ZstCable * cable)
{
	if (cable != NULL) {
		for (vector<ZstCable*>::iterator cable_iter = m_cables.begin(); cable_iter != m_cables.end(); ++cable_iter) {
			if ((*cable_iter) == cable) {
				m_cables.erase(cable_iter);
				break;
			}
		}
		delete cable;
	}
}

std::vector<ZstCable*>& ZstClient::cables()
{
	return m_cables;
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
