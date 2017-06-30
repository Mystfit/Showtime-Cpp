#include "Showtime.h"
#include "ZstPerformer.h"
#include "ZstEndpoint.h"
#include "ZstActor.h"
#include "ZstPlug.h"
#include "ZstURI.h"
#include "ZstURIWire.h"
#include "ZstMessages.h"
#include "ZstEvent.h"

using namespace std;

ZstEndpoint::ZstEndpoint() {
}

ZstEndpoint::~ZstEndpoint() {
	destroy();
}

void ZstEndpoint::destroy() {
	//Only need to call cleanup once
	if (m_is_ending)
		return;

	m_is_ending = true;
	m_connected_to_stage = false;
	ZstActor::destroy();
	zsock_destroy(&m_stage_requests);
	zsock_destroy(&m_stage_updates);
	zsock_destroy(&m_stage_router);
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
	zsys_shutdown();
}


//ZstEndpoint
void ZstEndpoint::init()
{
    cout << "Starting Showtime" << endl;
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
    
    ziflist_t * interfaces = ziflist_new();
    const char * net_if = ziflist_first(interfaces);
    const char * interface_ip = ziflist_address(interfaces);
    
    //Pick first publicly available IP by default.
    m_network_interface = string(net_if);
    string network_ip = string(interface_ip);
    
    cout << "ZST: Network interfaces on system:" << endl;
    while (net_if) {
        interface_ip = ziflist_address(interfaces);
        if(interface_ip){
            cout << "ZST: " << net_if << " " << interface_ip << endl;
        }
		net_if = ziflist_next(interfaces);
    }
    
    cout << "ZST: Using network interface: " << m_network_interface << endl;
    
    char client_bind_addr[100];
    sprintf(client_bind_addr, "@tcp://%s:*", network_ip.c_str());
    
    //Graph output socket
    m_graph_out = zsock_new_pub(client_bind_addr);
    m_output_endpoint = zsock_last_endpoint(m_graph_out);
    
    cout << "ZST: Endpoint graph address: " << m_output_endpoint << endl;
    
    if(m_graph_out)
        zsock_set_linger(m_graph_out, 0);
    
    start();
}

void ZstEndpoint::start() {
	ZstActor::start();
}

void ZstEndpoint::stop() {
	ZstActor::stop();
}


// ---------------
// Socket handlers
// ---------------
int ZstEndpoint::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
	ZstEndpoint *endpoint = (ZstEndpoint*)arg;
    zmsg_t *msg = endpoint->receive_from_graph();
    ZstURI sender = ZstURI::from_char(zmsg_popstr(msg));

	msgpack::object_handle result;
	zframe_t * payload = zmsg_pop(msg);

	unpack(result, (char*)zframe_data(payload), zframe_size(payload));
    msgpack::object obj = result.get();
	
	endpoint->broadcast_to_local_plugs(sender, obj);

	zmsg_destroy(&msg);
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
	zsock_connect(m_graph_in, performer_args.endpoint.c_str());
	zsock_set_subscribe(m_graph_in, "");

	ZstCable cable = ZstCable(performer_args.output_plug, performer_args.input_plug);
	auto it = find_if(m_cables.begin(), m_cables.end(), [&cable](ZstCable * current) {
		return *current == cable;
	});

	//Check if cable already exists
	if (it != m_cables.end()) {
		cout << "ZST: Cable already exists. Ignoring." << endl;
		return;
	}
	m_cables.push_back(new ZstCable(performer_args.output_plug, performer_args.input_plug));
}

int ZstEndpoint::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((ZstEndpoint*)arg)->ping_stage();
	return 0;
}

void ZstEndpoint::register_endpoint_to_stage(std::string stage_address) {
	m_stage_addr = string(stage_address);

	//Build endpoint addresses
	char stage_req_addr[100];
	sprintf(stage_req_addr, "tcp://%s:%d", stage_address.c_str(), STAGE_REP_PORT);

	char stage_sub_addr[100];
	sprintf(stage_sub_addr, "tcp://%s:%d", stage_address.c_str(), STAGE_PUB_PORT);

	//Stage request socket for querying the performance
	m_stage_requests = zsock_new_req(stage_req_addr);
	zsock_set_linger(m_stage_requests, 0);

	ZstMessages::RegisterEndpoint args;
	args.uuid = zuuid_str(m_startup_uuid);
	args.address = m_output_endpoint;

	cout << "ZST: Registering endpoint" << endl;
	send_to_stage(ZstMessages::build_message<ZstMessages::RegisterEndpoint>(ZstMessages::Kind::STAGE_REGISTER_ENDPOINT, args));

	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type == ZstMessages::Kind::STAGE_REGISTER_ENDPOINT_ACK) {

		ZstMessages::RegisterEndpointAck endpoint_ack = ZstMessages::unpack_message_struct<ZstMessages::RegisterEndpointAck>(responseMsg);
		cout << "ZST: Successfully registered endpoint to stage. UUID is " << endpoint_ack.assigned_uuid << endl;

		//Connect performer dealer to stage now that it's been registered successfully
		char stage_router_addr[30];
		sprintf(stage_router_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_ROUTER_PORT);

		m_assigned_uuid = endpoint_ack.assigned_uuid;
		zsock_set_identity(m_stage_router, endpoint_ack.assigned_uuid.c_str());
		zsock_connect(m_stage_router, stage_router_addr);

		//Stage sub socket for update messages
		cout << "ZST: Connecting to stage publisher " << stage_sub_addr << endl;
		zsock_connect(m_stage_updates, stage_sub_addr);
		zsock_set_subscribe(m_stage_updates, "");

		//TODO: Need to check handshake before setting connection as active
		m_connected_to_stage = true;
		signal_sync();
	}
	else {
        throw runtime_error("ZST: Stage performer registration responded with error -> Kind: " + std::to_string((int)message_type));
	}

	zmsg_destroy(&responseMsg);
}

void ZstEndpoint::signal_sync()
{
	if (m_connected_to_stage) {
		cout << "ZST: Requesting stage snapshot" << endl;
		send_through_stage(ZstMessages::build_signal(ZstMessages::Signal::SYNC));
	}
}

void ZstEndpoint::stage_update_handler(zsock_t * socket, zmsg_t * msg)
{
	ZstMessages::StageUpdates update_args = ZstMessages::unpack_message_struct<ZstMessages::StageUpdates>(msg);
	for (vector<ZstEvent>::iterator event_iter = update_args.updates.begin(); event_iter != update_args.updates.end(); ++event_iter) {
		if ((*event_iter).get_update_type() == ZstEvent::CABLE_DESTROYED) {
			//Remove any cables that we own that have been destroyed
			
			ZstCable * cable = get_cable_by_URI((*event_iter).get_first(), (*event_iter).get_second());
			if (cable != NULL) {
				remove_cable(cable);
			}
		}
		Showtime::endpoint().enqueue_plug_event(ZstEvent((*event_iter).get_first(), (*event_iter).get_update_type()));
	}
}

bool ZstEndpoint::is_connected_to_stage()
{
	return m_connected_to_stage;
}

void ZstEndpoint::register_performer_to_stage(string performer) {
	ZstMessages::RegisterPerformer register_args;

	// Need to send back our assigned uuid so we can attach the new performer to our endpoint on the stage
	register_args.endpoint_uuid = m_assigned_uuid;
	register_args.name = performer;
	send_to_stage(ZstMessages::build_message<ZstMessages::RegisterPerformer>(ZstMessages::Kind::STAGE_REGISTER_PERFORMER, register_args));
	check_stage_response_ok();
}

ZstPerformer * ZstEndpoint::create_performer(ZstURI uri)
{
	ZstPerformer * perf = new ZstPerformer(uri.performer());
	m_performers[uri.performer()] = perf;
	register_performer_to_stage(perf->get_name());
	return perf;
}

ZstPerformer * ZstEndpoint::get_performer_by_URI(const ZstURI uri)
{
	return m_performers[uri.performer()];
}

template ZstIntPlug* ZstEndpoint::create_plug<ZstIntPlug>(ZstURI * uri);
template<typename T>
 T* ZstEndpoint::create_plug(ZstURI * uri) {

	ZstMessages::RegisterPlug plug_args;
	plug_args.address = ZstURIWire(*(uri));
	zmsg_t * plug_msg = ZstMessages::build_message<ZstMessages::RegisterPlug>(ZstMessages::Kind::STAGE_REGISTER_PLUG, plug_args);
	Showtime::endpoint().send_to_stage(plug_msg);

	if (check_stage_response_ok()) {
		T *plug = new T(uri);
		Showtime::endpoint().get_performer_by_URI(*uri)->add_plug(plug);
		return plug;
	}
	return NULL;
}

 ZstIntPlug * ZstEndpoint::create_int_plug(ZstURI * uri)
 {
	 return create_plug<ZstIntPlug>(uri);
 }

 int ZstEndpoint::destroy_plug(ZstPlug * plug)
 {
	 ZstMessages::DestroyPlug destroy_args;
	 destroy_args.address = ZstURIWire(*(plug->get_URI()));
	 send_to_stage(ZstMessages::build_message<ZstMessages::DestroyPlug>(ZstMessages::Kind::STAGE_DESTROY_PLUG, destroy_args));

	 ZstMessages::Signal s = check_stage_response_ok();
	 if (s){
		 m_performers[plug->get_URI()->performer()]->remove_plug(plug);
		 delete plug;
	 }
	 return (int)s;
 }


 int ZstEndpoint::connect_cable(const ZstURI * a, const ZstURI * b)
{
	ZstMessages::PlugConnection plug_args;
	plug_args.first = *a;
	plug_args.second = *b;
	send_to_stage(ZstMessages::build_message<ZstMessages::PlugConnection>(ZstMessages::Kind::STAGE_REGISTER_CABLE, plug_args));
	return check_stage_response_ok();
}

int ZstEndpoint::destroy_cable(const ZstURI * a, const ZstURI * b)
{
	ZstMessages::PlugConnection plug_args;
	plug_args.first = *a;
	plug_args.second = *b;
	send_to_stage(ZstMessages::build_message<ZstMessages::PlugConnection>(ZstMessages::Kind::STAGE_DESTROY_CABLE, plug_args));
	return check_stage_response_ok();
}


ZstCable * ZstEndpoint::get_cable_by_URI(const ZstURI & uriA, const ZstURI & uriB) {

	auto it = find_if(m_cables.begin(), m_cables.end(), [&uriA, &uriB](ZstCable * current) {
		cout << ((*current == uriA) && (*current == uriB)) << endl;
		return (*current == uriA) && (*current == uriB);
	});

	if (it != m_cables.end()) {
		return (*it);
	}
	return NULL;
}

void ZstEndpoint::remove_cable(ZstCable * cable)
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


chrono::milliseconds ZstEndpoint::ping_stage()
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
	return delta;
}

void ZstEndpoint::enqueue_plug_event(ZstEvent event)
{
	m_events.push(event);
}

ZstEvent ZstEndpoint::pop_plug_event()
{
	return m_events.pop();
}

int ZstEndpoint::plug_event_queue_size()
{
	return m_events.size();
}

void ZstEndpoint::attach_stage_event_callback(ZstEventCallback *callback) {
	m_stage_callbacks.push_back(callback);
}

void ZstEndpoint::destroy_stage_event_callback(ZstEventCallback *callback) {
	m_stage_callbacks.erase(std::remove(m_stage_callbacks.begin(), m_stage_callbacks.end(), callback), m_stage_callbacks.end());
	delete callback;
}

void ZstEndpoint::run_stage_event_callbacks(ZstEvent e) {
	if (m_stage_callbacks.size() > 0) {
		for (vector<ZstEventCallback*>::iterator callback = m_stage_callbacks.begin(); callback != m_stage_callbacks.end(); ++callback) {
			(*callback)->run(e);
		}
	}
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
	zmsg_pop(msg);

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
	return s;
}

void ZstEndpoint::broadcast_to_local_plugs(ZstURI output_plug, msgpack::object obj) {

	for (vector <ZstCable*>::iterator cable_iter = m_cables.begin(); cable_iter != m_cables.end(); ++cable_iter) {
		if ((*cable_iter)->get_output() == output_plug) {
			ZstPerformer* performer = get_performer_by_URI((*cable_iter)->get_input());
			ZstPlug * plug = performer->get_plug_by_URI((*cable_iter)->get_input());
			if (plug != NULL) {
				plug->recv(obj);
			}
			else {
				cout << "ZST: Ignoring plug hit from " << (*cable_iter)->get_output().to_char()
					<< " for missing input plug " << (*cable_iter)->get_input().to_char() <<  endl;
			}
		}
	}
}
