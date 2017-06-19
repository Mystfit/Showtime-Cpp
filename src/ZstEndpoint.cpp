#include "Showtime.h"
#include "ZstPerformer.h"
#include "ZstEndpoint.h"
#include "ZstActor.h"
#include "ZstPlug.h"
#include "ZstURI.h"
#include "ZstMessages.h"

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
		attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
	}

	//Graph input socket
	m_graph_in = zsock_new(ZMQ_SUB);
	if (m_graph_in) {
		zsock_set_linger(m_graph_in, 0);
		attach_pipe_listener(m_graph_in, s_handle_graph_in, this);
	}

	//Graph output socket
	cout << "!!!FIXME!!! Setting pub socket interface to localhost due to binding to all interfaces not working in Windows" << endl;
	m_graph_out = zsock_new_pub("@tcp://127.0.0.1:*");
	m_output_endpoint = zsock_last_endpoint(m_graph_out);

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
    
	cout << "PERFORMER: Recieved graph message" << endl;

    ZstURI sender = ZstURI::from_str(zmsg_popstr(msg));

	msgpack::object_handle result;
	zframe_t * payload = zmsg_pop(msg);

	unpack(result, (char*)zframe_data(payload), zframe_size(payload));
    msgpack::object obj = result.get();
	
    cout << "Endpoint received a published message: " << endpoint->m_assigned_uuid << endl;
    cout << "Message is from " << sender.to_str() << endl;
	endpoint->broadcast_to_local_plugs(sender, obj);
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
		default:
			cout << "Didn't understand message of type " << message_type << endl;
            break;
    }
    
    return 0;
}

void ZstEndpoint::connect_performer_handler(zsock_t * socket, zmsg_t * msg) {
	ZstMessages::PerformerConnection performer_args = ZstMessages::unpack_message_struct<ZstMessages::PerformerConnection>(msg);

	//Connect to endpoint publisher
	zsock_connect(m_graph_in, performer_args.endpoint.c_str());
	zsock_set_subscribe(m_graph_in, "");

	//Register local connections so we can take the single published message and make sure each expecting internal input
	//plug receives it
	ZstPlug * input = get_performer_by_name(performer_args.input_plug.performer())->get_plug_by_name(performer_args.input_plug.name());
	m_plug_connections[performer_args.output_plug].push_back(input);

	cout << "PERFORMER: Connecting to " << performer_args.endpoint << ". My output endpoint is " << m_output_endpoint << endl;
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

	//Stage request socket for querying the performance
	m_stage_requests = zsock_new_req(stage_req_addr);
	zsock_set_linger(m_stage_requests, 0);

	ZstMessages::RegisterEndpoint args;
	args.uuid = zuuid_str(m_startup_uuid);
	args.address = m_output_endpoint;

	cout << "PERFORMER: Registering endpoint" << endl;
	send_to_stage(ZstMessages::build_message<ZstMessages::RegisterEndpoint>(ZstMessages::Kind::STAGE_REGISTER_ENDPOINT, args));

	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type == ZstMessages::Kind::STAGE_REGISTER_ENDPOINT_ACK) {

		ZstMessages::RegisterEndpointAck endpoint_ack = ZstMessages::unpack_message_struct<ZstMessages::RegisterEndpointAck>(responseMsg);
		cout << "PERFORMER: Successfully registered endpoint to stage. UUID is " << endpoint_ack.assigned_uuid << endl;

		//Connect performer dealer to stage now that it's been registered successfully
		char stage_router_addr[30];
		sprintf(stage_router_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_ROUTER_PORT);

		m_assigned_uuid = endpoint_ack.assigned_uuid;
		zsock_set_identity(m_stage_router, endpoint_ack.assigned_uuid.c_str());
		zsock_connect(m_stage_router, stage_router_addr);

		m_connected_to_stage = true;
	}
	else {
        throw runtime_error("PERFORMER: Stage performer registration responded with error -> Kind: " + std::to_string((int)message_type));
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

	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);

	if (message_type == ZstMessages::Kind::SIGNAL) {
		ZstMessages::Signal s = ZstMessages::unpack_signal(responseMsg);
		if(s != ZstMessages::Signal::OK)
            throw runtime_error("PERFORMER: Performer registration responded with message other than OK -> Signal" + std::to_string((int)s));
	}
}

ZstPerformer * ZstEndpoint::create_performer(std::string name)
{
	ZstPerformer * perf = new ZstPerformer(name);
	m_performers[name] = perf;
	register_performer_to_stage(perf->get_name());
	return perf;
}

ZstPerformer * ZstEndpoint::get_performer_by_name(std::string performer)
{
	return m_performers[performer];
}

template ZstIntPlug* ZstEndpoint::create_plug<ZstIntPlug>(ZstURI * uri);
template<typename T>
 T* ZstEndpoint::create_plug(ZstURI * uri) {

	ZstMessages::RegisterPlug plug_args;
	plug_args.address = *(uri);
	zmsg_t * plug_msg = ZstMessages::build_message<ZstMessages::RegisterPlug>(ZstMessages::Kind::STAGE_REGISTER_PLUG, plug_args);
	Showtime::endpoint().send_to_stage(plug_msg);

	zmsg_t *responseMsg = Showtime::endpoint().receive_from_stage();

	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type == ZstMessages::Kind::SIGNAL) {
		ZstMessages::Signal s = ZstMessages::unpack_signal(responseMsg);
		if (s != ZstMessages::Signal::OK)
            throw runtime_error("PERFORMER: Plug creation responded with message other than OK -> Signal" + std::to_string((int)s));
    }

	T *plug = new T(uri);
	Showtime::endpoint().get_performer_by_name(uri->performer())->add_plug(plug);
	return plug;
}

 ZstIntPlug * ZstEndpoint::create_int_plug(ZstURI * uri)
 {
	 return create_plug<ZstIntPlug>(uri);
 }

 void ZstEndpoint::destroy_plug(ZstPlug * plug)
{
	ZstMessages::DestroyPlug destroy_args;
	destroy_args.address = *(plug->get_URI());
	send_to_stage(ZstMessages::build_message<ZstMessages::DestroyPlug>(ZstMessages::Kind::STAGE_DESTROY_PLUG, destroy_args));

	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type != ZstMessages::Kind::SIGNAL) {
		ZstMessages::Signal s = ZstMessages::unpack_signal(responseMsg);
		if (s != ZstMessages::Signal::OK)
            throw runtime_error("PERFORMER: Plug deletion responded with message other than OK -> Signal:" + std::to_string((int)s));
    }
	m_performers[plug->get_URI()->performer()]->remove_plug(plug);
	delete plug;
}


std::vector<ZstURI> ZstEndpoint::get_all_plug_URIs(string performer, string instrument) {
	ZstMessages::ListPlugs plug_args;
	plug_args.performer = performer;
	plug_args.instrument = instrument;
	send_to_stage(ZstMessages::build_message<ZstMessages::ListPlugs>(ZstMessages::Kind::STAGE_LIST_PLUGS, plug_args));

	zmsg_t *responseMsg = receive_from_stage();

	ZstMessages::ListPlugsAck plugResponse;

	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type == ZstMessages::Kind::STAGE_LIST_PLUGS_ACK) {
		plugResponse = ZstMessages::unpack_message_struct<ZstMessages::ListPlugsAck>(responseMsg);
		for (vector<ZstURI>::iterator plugIter = plugResponse.plugs.begin(); plugIter != plugResponse.plugs.end(); ++plugIter) {
			cout << "PERFORMER: Remote plug: " << plugIter->performer() << "/" << plugIter->instrument() << "/" << plugIter->name() << endl;
		}
	}
	else {
        throw runtime_error("PERFORMER: Plug list responded with message other than OK -> Kind: " + std::to_string((int)message_type));
    }
	cout << "PERFORMER: Total returned remote plugs: " << plugResponse.plugs.size() << endl;

	return plugResponse.plugs;
}

std::vector<std::pair<ZstURI, ZstURI> > ZstEndpoint::get_all_plug_connections(std::string performer, std::string instrument)
{
	ZstMessages::ListPlugs plug_args;
	plug_args.performer = performer;
	plug_args.instrument = instrument;
	send_to_stage(ZstMessages::build_message<ZstMessages::ListPlugs>(ZstMessages::Kind::STAGE_LIST_PLUG_CONNECTIONS, plug_args));

	zmsg_t *responseMsg = receive_from_stage();

	ZstMessages::ListPlugConnectionsAck plugResponse;

	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type == ZstMessages::Kind::STAGE_LIST_PLUG_CONNECTIONS_ACK) {
		plugResponse = ZstMessages::unpack_message_struct<ZstMessages::ListPlugConnectionsAck>(responseMsg);
	}
	else {
        throw runtime_error("PERFORMER: Plug list connections responded with error -> Kind: " + std::to_string((int)message_type));
	}
	cout << "PERFORMER: Total returned plug connections: " << plugResponse.plug_connections.size() << endl;

	return plugResponse.plug_connections;
}


void ZstEndpoint::connect_plugs(const ZstURI * a, const ZstURI * b)
{
	ZstMessages::ConnectPlugs plug_args;
	plug_args.first = *a;
	plug_args.second = *b;
	send_to_stage(ZstMessages::build_message<ZstMessages::ConnectPlugs>(ZstMessages::Kind::STAGE_REGISTER_CONNECTION, plug_args));

	zmsg_t *responseMsg = receive_from_stage();

	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type == ZstMessages::Kind::SIGNAL) {
		ZstMessages::Signal s = ZstMessages::unpack_signal(responseMsg);
		if (s != ZstMessages::Signal::OK)
            throw runtime_error("PERFORMER: Plug connection responded with message other than OK -> Signal: " + std::to_string((int)s));
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
	
	//Get stage response
	zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
	
	//Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type == ZstMessages::Kind::SIGNAL) {
		end = chrono::system_clock::now();
		delta = chrono::duration_cast<chrono::milliseconds>(end - start);
		cout << "PERFORMER: Client received heartbeat ping ack. Roundtrip was " << delta.count() << "ms" << endl;
	}
	else {
        throw runtime_error("PERFORMER: Stage ping responded with message other than OK -> Kind: " + std::to_string(message_type));
	}
	
	return delta;
}

void ZstEndpoint::enqueue_plug_event(PlugEvent event)
{
	std::cout << "About to enqueue" << std::endl;
	m_plug_events.push(event);
}

PlugEvent ZstEndpoint::pop_plug_event()
{
	return m_plug_events.pop();
}

int ZstEndpoint::plug_event_queue_size()
{
	return m_plug_events.size();
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

zmsg_t * ZstEndpoint::receive_routed_from_stage() {
	zmsg_t * msg = zmsg_recv(m_stage_router);

	//Pop blank seperator frame
	zmsg_pop(msg);

	return msg;
}

zmsg_t * ZstEndpoint::receive_from_graph() {
	return zmsg_recv(m_graph_in);
}


void ZstEndpoint::broadcast_to_local_plugs(ZstURI output_plug, msgpack::object obj) {

    cout << "Looking for local plugs connected to incoming output plug" << endl;
    cout << "There are currently " << m_plug_connections[output_plug].size() << " local plugs attached to this output" << endl;
	for (vector<ZstPlug*>::iterator plug_iter = m_plug_connections[output_plug].begin(); plug_iter != m_plug_connections[output_plug].end(); ++plug_iter) {
        cout << "Forwarding message to " << (*plug_iter)->get_URI()->name() << endl;
		(*plug_iter)->recv(obj);
	}
}
