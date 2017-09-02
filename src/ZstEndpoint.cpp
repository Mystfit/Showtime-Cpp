#include <chrono>

#include "Showtime.h"
#include "ZstPerformer.h"
#include "ZstEndpoint.h"
#include "ZstActor.h"
#include "ZstPlug.h"
#include "ZstMessages.h"
#include "ZstURIWire.h"
#include "ZstEventWire.h"
#include "ZstPlugEvent.h"
#include "ZstValueWire.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstFilter.h"

using namespace std;

ZstEndpoint::ZstEndpoint() {

}

ZstEndpoint::~ZstEndpoint() {
	destroy();
}

void ZstEndpoint::destroy() {
	//Only need to call cleanup once
	if (m_is_ending || m_is_destroyed)
		return;
    m_is_ending = true;

	leave_stage();

	delete m_performer_arriving_event_manager;
	delete m_performer_leaving_event_manager;
	delete m_cable_arriving_event_manager;
	delete m_cable_leaving_event_manager;
	delete m_plug_arriving_event_manager;
	delete m_plug_leaving_event_manager;

	ZstActor::destroy();
	zsock_destroy(&m_stage_requests);
	zsock_destroy(&m_stage_updates);
	zsock_destroy(&m_stage_router);
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
	zsys_shutdown();
	m_is_ending = false;
	m_is_destroyed = true;
}

void ZstEndpoint::init()
{
    cout << "Starting Showtime" << endl;

	if (m_is_ending) {
		return;
	}

	m_is_destroyed = false;

	m_performer_arriving_event_manager = new ZstCallbackQueue<ZstEntityEventCallback, ZstURI>();
	m_performer_leaving_event_manager = new ZstCallbackQueue<ZstEntityEventCallback, ZstURI>();
	m_cable_arriving_event_manager = new ZstCallbackQueue<ZstCableEventCallback, ZstCable>();
	m_cable_leaving_event_manager = new ZstCallbackQueue<ZstCableEventCallback, ZstCable>();
	m_plug_arriving_event_manager = new ZstCallbackQueue<ZstPlugEventCallback, ZstURI>();
	m_plug_leaving_event_manager = new ZstCallbackQueue<ZstPlugEventCallback, ZstURI>();

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
    sprintf(m_graph_out_addr, "@tcp://%s:*", network_ip.c_str());
    m_graph_out = zsock_new_pub(m_graph_out_addr);
    m_output_endpoint = zsock_last_endpoint(m_graph_out);
    cout << "ZST: Endpoint graph address: " << m_output_endpoint << endl;
    
    if(m_graph_out)
        zsock_set_linger(m_graph_out, 0);
    
    start();
}

void ZstEndpoint::process_callbacks()
{
	while (m_events.size() > 0) {
		ZstEvent * e = pop_event();

		switch (e->get_update_type()) {
		case ZstEvent::EventType::PLUG_HIT:
            {
                ZstURI entity_URI = e->get_first();
                ZstURI entity_parent = entity_URI.range(0, entity_URI.size() - 1);
                
                ZstComponent * component = dynamic_cast<ZstComponent*>(get_entity_by_URI(entity_parent));
                if (component != NULL) {
                    ZstInputPlug * plug = (ZstInputPlug*)component->get_plug_by_URI(entity_URI);
                    if (plug != NULL) {
                        plug->recv(((ZstPlugEvent*)e)->value());
                        plug->m_input_fired_manager->run_event_callbacks(plug);
                        
                        //Pre-emptively delete event whilst we know it's a plug event
                        delete (ZstPlugEvent*)e;
                        e = 0;
                    }
                }
            }
			break;
		case ZstEvent::CABLE_CREATED:
			cable_arriving_events()->run_event_callbacks(ZstCable(e->get_first(), e->get_second()));
			break;
		case ZstEvent::CABLE_DESTROYED:
            {
                ZstCable * cable = get_cable_by_URI(e->get_first(), e->get_second());
                if (cable != NULL) {
                    remove_cable(cable);
                    cable_leaving_events()->run_event_callbacks(ZstCable(e->get_first(), e->get_second()));
                }
            }
			break;
		case ZstEvent::PLUG_CREATED:
			plug_arriving_events()->run_event_callbacks(e->get_first());
			break;
		case ZstEvent::PLUG_DESTROYED:
			plug_leaving_events()->run_event_callbacks(e->get_first());
			break;
		case ZstEvent::ENTITY_CREATED:
			entity_arriving_events()->run_event_callbacks(e->get_first());
			break;
		case ZstEvent::ENTITY_DESTROYED:
			entity_arriving_events()->run_event_callbacks(e->get_first());
			break;
		default:
			break;
		}

		//Delete event pointer
		delete e;
	}
}

string ZstEndpoint::first_available_ext_ip(){
    ziflist_t * interfaces = ziflist_new();
    const char * net_if = ziflist_first(interfaces);
    const char * interface_ip = ziflist_address(interfaces);
    
    if(net_if == NULL) {
        interface_ip = "127.0.0.1";
    }
    
    string interface_ip_str = string(interface_ip);
    
	//TODO: Figure out how to clean these up on Windows - DLL boundary issue?
    //delete[] interface_ip;
    //delete[] net_if;
    
    return interface_ip_str;
}

void ZstEndpoint::start() {
	ZstActor::start();
}

void ZstEndpoint::stop() {
	ZstActor::stop();
}


// -------------
// Endpoint init
// -------------

void ZstEndpoint::register_endpoint_to_stage(std::string stage_address) {
	m_stage_addr = string(stage_address);

	//Build endpoint addresses
	sprintf(m_stage_requests_addr, "tcp://%s:%d", stage_address.c_str(), STAGE_REP_PORT);

	//Stage request socket for querying the performance
	if (m_stage_requests == NULL) {
		m_stage_requests = zsock_new(ZMQ_REQ);
		zsock_set_linger(m_stage_requests, 0);
	}
	zsock_connect(m_stage_requests, "%s", m_stage_requests_addr);

	ZstMessages::CreateEndpoint args;
	args.uuid = zuuid_str(m_startup_uuid);
	args.address = m_output_endpoint;

	cout << "ZST: Registering endpoint" << endl;
	send_to_stage(ZstMessages::build_message<ZstMessages::CreateEndpoint>(ZstMessages::Kind::STAGE_CREATE_ENDPOINT, args));

	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type == ZstMessages::Kind::STAGE_CREATE_ENDPOINT_ACK) {

		ZstMessages::CreateEndpointAck endpoint_ack = ZstMessages::unpack_message_struct<ZstMessages::CreateEndpointAck>(responseMsg);
		cout << "ZST: Successfully registered endpoint to stage. UUID is " << endpoint_ack.assigned_uuid << endl;

		//Connect performer dealer to stage now that it's been registered successfully
		sprintf(m_stage_router_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_ROUTER_PORT);
		m_assigned_uuid = endpoint_ack.assigned_uuid;
		zsock_set_identity(m_stage_router, endpoint_ack.assigned_uuid.c_str());
		zsock_connect(m_stage_router, "%s", m_stage_router_addr);

		//Stage sub socket for update messages
		sprintf(m_stage_updates_addr, "tcp://%s:%d", stage_address.c_str(), STAGE_PUB_PORT);
		cout << "ZST: Connecting to stage publisher " << m_stage_updates_addr << endl;
		zsock_connect(m_stage_updates, "%s", m_stage_updates_addr);
		zsock_set_subscribe(m_stage_updates, "");

		//TODO: Need to check handshake before setting connection as active
		m_connected_to_stage = true;
		signal_sync();

		//Set up heartbeat timer
		m_heartbeat_timer_id = attach_timer(s_heartbeat_timer, HEARTBEAT_DURATION, this);
	}
	else {
		throw runtime_error("ZST: Stage performer registration responded with error -> Kind: " + std::to_string((int)message_type));
	}

	zmsg_destroy(&responseMsg);
}

const char * ZstEndpoint::get_endpoint_UUID() const
{
	return m_assigned_uuid.c_str();
}

void ZstEndpoint::signal_sync()
{
	if (m_connected_to_stage) {
		cout << "ZST: Requesting stage snapshot" << endl;
		send_through_stage(ZstMessages::build_signal(ZstMessages::Signal::SYNC));
	}
}


// ---------------
// Socket handlers
// ---------------
int ZstEndpoint::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
	ZstEndpoint *endpoint = (ZstEndpoint*)arg;
    zmsg_t *msg = zmsg_recv(endpoint->m_graph_in);
    
    //Get sender from msg
	zframe_t * sender_frame = zmsg_pop(msg);
	char * sender_s = zframe_strdup(sender_frame);
    ZstURI sender = ZstURI(sender_s);
	zstr_free(&sender_s);
	zframe_destroy(&sender_frame);
    
    //Get payload from msg
    ZstValue value = (ZstValue)ZstMessages::unpack_message_struct<ZstValueWire>(msg);
    
    zmsg_destroy(&msg);
    free(msg);
    
	endpoint->broadcast_to_local_plugs(sender, value);
	return 0;
}

int ZstEndpoint::s_handle_stage_update_in(zloop_t * loop, zsock_t * socket, void * arg) {
	ZstEndpoint *endpoint = (ZstEndpoint*)arg;

	zmsg_t *msg = endpoint->receive_stage_update();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);

	switch (message_type) {
	case ZstMessages::Kind::STAGE_UPDATE:
		endpoint->stage_update_handler(socket, msg);
		break;
	default:
		cout << "ZST: Stage subscriber - Didn't understand message of type " << message_type << endl;
		break;
	}

	zmsg_destroy(&msg);
	return 0;
}

int ZstEndpoint::s_handle_stage_router(zloop_t * loop, zsock_t * socket, void * arg){
	ZstEndpoint *endpoint = (ZstEndpoint*)arg;

    zmsg_t *msg = endpoint->receive_routed_from_stage();
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);

    switch(message_type){
        case ZstMessages::Kind::PERFORMER_REGISTER_CONNECTION:
			endpoint->connect_performer_handler(socket, msg);
            break;
		case ZstMessages::Kind::STAGE_UPDATE:
			endpoint->stage_update_handler(socket, msg);
			break;
		default:
			cout << "ZST: Performer dealer - Didn't understand message of type " << message_type << endl;
            break;
    }
    
	zmsg_destroy(&msg);
    return 0;
}

void ZstEndpoint::connect_performer_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::PerformerConnection performer_args = ZstMessages::unpack_message_struct<ZstMessages::PerformerConnection>(msg);

	cout << "ZST: Connecting to " << performer_args.endpoint << ". My output endpoint is " << m_output_endpoint << endl;

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, "%s", performer_args.endpoint.c_str());
	zsock_set_subscribe(m_graph_in, "");

	ZstCable cable = ZstCable(performer_args.output_plug, performer_args.input_plug);
	auto it = find_if(m_local_cables.begin(), m_local_cables.end(), [&cable](ZstCable * current) {
		return *current == cable;
	});

	//Check if cable already exists
	if (it != m_local_cables.end()) {
		cout << "ZST: Cable already exists. Ignoring." << endl;
		return;
	}
	m_local_cables.push_back(new ZstCable(performer_args.output_plug, performer_args.input_plug));
}

int ZstEndpoint::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
	ZstEndpoint * endpoint = (ZstEndpoint*)arg;
	endpoint->send_through_stage(ZstMessages::build_signal(ZstMessages::Signal::HEARTBEAT));
	return 0;
}

std::map<ZstURI, ZstEntityBase*>& ZstEndpoint::entities()
{
	return m_entities;
}

std::vector<ZstCable*>& ZstEndpoint::cables()
{
	return m_local_cables;
}

void ZstEndpoint::stage_update_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::StageUpdates update_args = ZstMessages::unpack_message_struct<ZstMessages::StageUpdates>(msg);
    for (auto event_iter : update_args.updates) {
		Showtime::endpoint().enqueue_event(new ZstEvent(event_iter.get_first(), event_iter.get_second(), event_iter.get_update_type()));
	}
}

bool ZstEndpoint::is_connected_to_stage()
{
	return m_connected_to_stage;
}

void ZstEndpoint::leave_stage()
{
	if (m_connected_to_stage) {
		cout << "ZST:Leaving stage" << endl;
		send_through_stage(ZstMessages::build_signal(ZstMessages::Signal::LEAVING));

		zsock_disconnect(m_stage_requests, "%s", m_stage_requests_addr);
		zsock_disconnect(m_stage_router, "%s", m_stage_router_addr);
		zsock_disconnect(m_stage_updates, "%s", m_stage_updates_addr);

		detach_timer(m_heartbeat_timer_id);

		m_connected_to_stage = false;
	}
}

void ZstEndpoint::register_entity_type(const char * entity_type)
{
	//throw std::exception("Register entity not implemented");
}


// --------
// Entities
// --------

int ZstEndpoint::register_entity(ZstEntityBase* entity)
{
    std::cout << "ZST_ENDPOINT: Registering entity " << entity->URI().path() << " to stage" << std::endl;

	int result = 0;
	ZstMessages::CreateEntity entity_args;
	entity_args.endpoint_uuid = get_endpoint_UUID();
	entity_args.entity_type = entity->entity_type();
	entity_args.address = ZstURIWire(entity->URI());
	zmsg_t * entity_msg = ZstMessages::build_message<ZstMessages::CreateEntity>(ZstMessages::Kind::STAGE_CREATE_ENTITY, entity_args);
	Showtime::endpoint().send_to_stage(entity_msg);
	
	if (check_stage_response_ok()) {
		m_entities[entity->URI()] = entity;
		result = 1;
	}

	return result;
}

int ZstEndpoint::destroy_entity(ZstEntityBase * entity)
{
	if (entity->is_destroyed() ||
		!entity->is_registered() ||
		m_is_destroyed
		)
	{
		return 0;
	}
		
	m_entities.erase(entity->URI());
	entity->set_destroyed();

	int result = 0;
	ZstMessages::DestroyURI destroy_args;
	destroy_args.address = ZstURIWire(entity->URI());
	send_to_stage(ZstMessages::build_message<ZstMessages::DestroyURI>(ZstMessages::Kind::STAGE_DESTROY_ENTITY, destroy_args));
	result = (int)check_stage_response_ok();
	return 1;
}

ZstEntityBase * ZstEndpoint::get_entity_by_URI(ZstURI uri)
{
	ZstEntityBase * result = NULL;

	auto entity_iter = m_entities.find(uri);
	if (entity_iter != m_entities.end()) {
		result = entity_iter->second;
	}

	return result;
}


// -----
// Plugs
// -----

template ZstInputPlug* ZstEndpoint::create_plug<ZstInputPlug>(ZstComponent* owner, const char * name, ZstValueType val_type, PlugDirection direction);
template ZstOutputPlug* ZstEndpoint::create_plug<ZstOutputPlug>(ZstComponent* owner, const char * name, ZstValueType val_type, PlugDirection direction);
template<typename T>
T* ZstEndpoint::create_plug(ZstComponent* owner, const char * name, ZstValueType val_type, PlugDirection direction) {
	T* plug = NULL;
	ZstURI address = ZstURI::join(owner->URI(), ZstURI(name));
	
	//Build message to register plug on stage
	ZstMessages::CreatePlug plug_args;
	plug_args.address = ZstURIWire(address);
    plug_args.dir = direction;
	zmsg_t * plug_msg = ZstMessages::build_message<ZstMessages::CreatePlug>(ZstMessages::Kind::STAGE_CREATE_PLUG, plug_args);
	Showtime::endpoint().send_to_stage(plug_msg);

	if (check_stage_response_ok()) {
		plug = new T(owner, name, val_type);
	}
	return plug;
}

 int ZstEndpoint::destroy_plug(ZstPlug * plug)
 {
	int status = 0;
	if (m_is_destroyed || plug->is_destroyed()) {
		return status;
	}
	plug->m_is_destroyed = true;

	 ZstMessages::DestroyURI destroy_args;
	 destroy_args.address = ZstURIWire(plug->get_URI());
	 send_to_stage(ZstMessages::build_message<ZstMessages::DestroyURI>(ZstMessages::Kind::STAGE_DESTROY_PLUG, destroy_args));

	 ZstMessages::Signal s = check_stage_response_ok();
	 if (s){
		 ZstComponent * component = dynamic_cast<ZstComponent*>(get_entity_by_URI(plug->get_URI()));
		 if (component) {
			 component->remove_plug(plug);
		 }
		 delete plug;
		 plug = 0;
		 status = 1;
	 }
	 return status;
 }


 // ------
 // Cables
 // ------
 
 int ZstEndpoint::connect_cable(ZstURI a, ZstURI b)
{
	ZstMessages::CreateCable plug_args;
	plug_args.first = ZstURIWire(a);
	plug_args.second = ZstURIWire(b);
	send_to_stage(ZstMessages::build_message<ZstMessages::CreateCable>(ZstMessages::Kind::STAGE_CREATE_CABLE, plug_args));
	return check_stage_response_ok();
}

int ZstEndpoint::destroy_cable(ZstURI a, ZstURI b)
{
	ZstMessages::CreateCable plug_args;
	plug_args.first = a;
	plug_args.second = b;
	send_to_stage(ZstMessages::build_message<ZstMessages::CreateCable>(ZstMessages::Kind::STAGE_DESTROY_CABLE, plug_args));
	return check_stage_response_ok();
}


vector<ZstCable*> ZstEndpoint::get_cables_by_URI(const ZstURI & uri) {

	vector<ZstCable*> cables;
	auto it = find_if(m_local_cables.begin(), m_local_cables.end(), [&uri](ZstCable* current) {
		return current->is_attached(uri);
	});
    
	for (it; it != m_local_cables.end(); ++it) {
		cables.push_back((*it));
	}

	return cables;
}

ZstCable * ZstEndpoint::get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB) {

	auto it = find_if(m_local_cables.begin(), m_local_cables.end(), [&uriA, &uriB](ZstCable * current) {
		return current->is_attached(uriA, uriB);
	});

	if (it != m_local_cables.end()) {
		return (*it);
	}
	return NULL;
}

void ZstEndpoint::remove_cable(ZstCable * cable)
{
	if (cable != NULL) {
		for (vector<ZstCable*>::iterator cable_iter = m_local_cables.begin(); cable_iter != m_local_cables.end(); ++cable_iter) {
			if ((*cable_iter) == cable) {
				m_local_cables.erase(cable_iter);
				break;
			}
		}
		delete cable;
	}
}


// ---

int ZstEndpoint::ping_stage()
{
	ZstMessages::Heartbeat beat;
	
	chrono::milliseconds delta = chrono::milliseconds(-1);
	chrono::time_point<chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	
	beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
	send_to_stage(ZstMessages::build_message<ZstMessages::Heartbeat>(ZstMessages::Kind::ENDPOINT_HEARTBEAT, beat));

	if (check_stage_response_ok()) {
		end = chrono::system_clock::now();
		delta = chrono::duration_cast<chrono::milliseconds>(end - start);
		cout << "ZST: Client received heartbeat ping ack. Roundtrip was " << delta.count() << "ms" << endl;
	}
	return (int)delta.count();
}


// ------
// Events
// ------

void ZstEndpoint::enqueue_event(ZstEvent * event)
{
	m_events.push(event);
}

ZstEvent * ZstEndpoint::pop_event()
{
	return m_events.pop();
}

int ZstEndpoint::event_queue_size()
{
	return m_events.size();
}


// ------------------------
// Callback manager getters
// ------------------------

ZstCallbackQueue<ZstEntityEventCallback, ZstURI> * ZstEndpoint::entity_arriving_events()
{
	return m_performer_arriving_event_manager;
}

ZstCallbackQueue<ZstEntityEventCallback, ZstURI> * ZstEndpoint::entity_leaving_events()
{
	return m_performer_leaving_event_manager;
}

ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * ZstEndpoint::plug_arriving_events()
{
	return m_plug_arriving_event_manager;
}

ZstCallbackQueue<ZstPlugEventCallback, ZstURI> * ZstEndpoint::plug_leaving_events()
{
	return m_plug_leaving_event_manager;
}

ZstCallbackQueue<ZstCableEventCallback, ZstCable>* ZstEndpoint::cable_arriving_events()
{
	return m_cable_arriving_event_manager;
}

ZstCallbackQueue<ZstCableEventCallback, ZstCable>* ZstEndpoint::cable_leaving_events()
{
	return m_cable_leaving_event_manager;
}


// ------------
// Send/Receive
// ------------

void ZstEndpoint::send_to_stage(zmsg_t * msg) {
	zmsg_send(&msg, m_stage_requests);
}

void ZstEndpoint::send_through_stage(zmsg_t * msg) {
	//Dealer socket doesn't add an empty frame to seperate identity chain and payload, so we handle it here
	zframe_t * empty = zframe_new_empty();
	zmsg_prepend(msg, &empty);
	zmsg_send(&msg, m_stage_router);
}

void ZstEndpoint::send_to_graph(zmsg_t * msg) {
	zmsg_send(&msg, m_graph_out);
}

zmsg_t * ZstEndpoint::receive_from_stage() {
	return zmsg_recv(m_stage_requests);
}

zmsg_t * ZstEndpoint::receive_stage_update() {
	return zmsg_recv(m_stage_updates);
}


zmsg_t * ZstEndpoint::receive_routed_from_stage() {
	zmsg_t * msg = zmsg_recv(m_stage_router);

	//Pop blank seperator frame
	zframe_t * empty = zmsg_pop(msg);
    zframe_destroy(&empty);

	return msg;
}

zmsg_t * ZstEndpoint::receive_from_graph() {
	return zmsg_recv(m_graph_in);
}

ZstMessages::Signal ZstEndpoint::check_stage_response_ok() {
	zmsg_t *responseMsg = Showtime::endpoint().receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type != ZstMessages::Kind::SIGNAL) {
		throw runtime_error("ZST: Attempting to check stage signal, but we got a message other than signal!");
	}

	ZstMessages::Signal s = ZstMessages::unpack_signal(responseMsg);
	if (s != ZstMessages::Signal::OK) {
		std::cout << "ZST: Stage responded with signal other than OK -> " << s << std::endl;
	}
    zmsg_destroy(&responseMsg);
	return s;
}

void ZstEndpoint::broadcast_to_local_plugs(ZstURI output_plug, ZstValue & value) {
	ZstCable * cable = NULL;

	for(int i = 0; i < cables().size(); ++i){
		cable = cables()[i];
		if (cable->get_output() == output_plug) {
			ZstEntityBase * entity = get_entity_by_URI(cable->get_input().range(0, cable->get_input().size() - 1));
			ZstComponent* component = dynamic_cast<ZstComponent*>(entity);
			if (component) {
				ZstInputPlug * plug = (ZstInputPlug*)component->get_plug_by_URI(cable->get_input());
				if (plug != NULL) {
					Showtime::endpoint().enqueue_event(new ZstPlugEvent(plug->get_URI(), value));
				}
				else {
					cout << "ZST: Ignoring plug hit from " << cable->get_output().path() << " for missing input plug " << cable->get_input().path() << endl;
				}
			}
		}
	}
}
