#include "Showtime.h"

using namespace std;

Showtime::Showtime(){
}

Showtime::~Showtime(){
	//Only need to call cleanup once
	if (!m_is_ending) {
		destroy();
		m_is_ending = true;
	}
}

void Showtime::destroy() {
	ZstActor::~ZstActor();
	zsock_destroy(&m_stage_requests);
	zsock_destroy(&m_stage_router);
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
	zsys_shutdown();
}

Showtime & Showtime::instance()
{
	static Showtime performance_singleton;
	return performance_singleton;
}

void Showtime::join(string stage_address){
	Showtime::instance().init(stage_address);
}

void Showtime::init(string stage_address)
{
	m_stage_addr = stage_address;

	//Build endpoint addresses
	char stage_req_addr[30];
	sprintf(stage_req_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_REP_PORT);

	//Stage request socket for querying the performance
	m_stage_requests = zsock_new_req(stage_req_addr);
	zsock_set_linger(m_stage_requests, 0);

	//Local dealer socket for receiving messages forwarded from other performers
	m_stage_router = zsock_new(ZMQ_DEALER);

	//Graph input socket
	m_graph_in = zsock_new(ZMQ_SUB);
	zsock_set_linger(m_graph_in, 0);
	attach_pipe_listener(m_graph_in, s_handle_graph_in, this);

	//Graph output socket
	cout << "!!!FIXME!!! Setting pub socket interface to localhost due to binding to all interfaces not working in Windows" << endl;
	m_graph_out = zsock_new_pub("@tcp://127.0.0.1:*");
	m_output_endpoint = zsock_last_endpoint(m_graph_out);
	zsock_set_linger(m_graph_out, 0);

	start();
}


// ---------------
// Local accessors
// ---------------

void Showtime::start(){
	ZstActor::start();
}

void Showtime::stop() {
	ZstActor::stop();
}


// ------------
// Send/Receive
// ------------

void Showtime::send_to_stage(zmsg_t * msg){
    zmsg_send(&msg, m_stage_requests);
}

void Showtime::send_through_stage(zmsg_t * msg){
    //Dealer socket doesn't add an empty frame to seperate identity chain and payload, so we handle it here
    zframe_t * empty = zframe_new_empty();
    zmsg_prepend(msg, &empty);
    zmsg_send(&msg, m_stage_router);
}

void Showtime::send_to_graph(zmsg_t * msg){
    zmsg_send(&msg, m_graph_out);
}

zmsg_t * Showtime::receive_from_stage(){
    return zmsg_recv(m_stage_requests);
}

zmsg_t * Showtime::receive_routed_from_stage(){
    zmsg_t * msg = zmsg_recv(m_stage_router);
 
    //Pop blank seperator frame
    zmsg_pop(msg);
    
    return msg;
}

zmsg_t * Showtime::receive_from_graph(){
    return zmsg_recv(m_graph_in);
}



// ---------------
// Socket handlers
// ---------------
int Showtime::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
    Showtime *performer = (Showtime*)arg;
    
    zmsg_t *msg = performer->receive_from_graph();
    string sender = zmsg_popstr(msg);

	msgpack::object_handle result;
	zframe_t * payload = zmsg_pop(msg);

	unpack(result, (char*)zframe_data(payload), zframe_size(payload));
	
	//Do stuff with message payload
	cout << "PERFORMER: Recieved graph message" << endl;

	return 0;
}

int Showtime::s_handle_stage_router(zloop_t * loop, zsock_t * socket, void * arg){
    Showtime *performer = (Showtime*)arg;
    
    zmsg_t *msg = performer->receive_routed_from_stage();
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    cout << "PERFORMER: Message from stage pipe: " << message_type << endl;
    
    switch(message_type){
        case ZstMessages::Kind::PERFORMER_REGISTER_CONNECTION:
            performer->connect_performer_handler(socket, msg);
            break;
    }
    
    return 0;
}

int Showtime::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((Showtime*)arg)->ping_stage();
	return 0;
}

void Showtime::connect_performer_handler(zsock_t * socket, zmsg_t * msg){
    ZstMessages::PerformerConnection performer_args = ZstMessages::unpack_message_struct<ZstMessages::PerformerConnection>(msg);
    
    zsock_connect(m_graph_in, performer_args.endpoint.c_str());
    zsock_set_subscribe(m_graph_in, "");
    cout << "PERFORMER: Connecting to " << performer_args.endpoint << ". My output endpoint is " << m_output_endpoint << endl;
}



// -------------
// API Functions
// -------------

void Showtime::register_endpoint_to_stage() {
	ZstMessages::RegisterEndpoint args;
	args.uuid = zuuid_str(m_startup_uuid);
	args.address = m_output_endpoint;
	cout << "PERFORMER: Registering performer" << endl;
	send_to_stage(ZstMessages::build_message<ZstMessages::RegisterEndpoint>(ZstMessages::Kind::STAGE_REGISTER_ENDPOINT, args));

	zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	
	if (message_type == ZstMessages::Kind::STAGE_REGISTER_ENDPOINT_ACK) {
		cout << "PERFORMER: Successfully registered endpoint to stage." << endl;

		ZstMessages::RegisterEndpointAck endpoint_ack = ZstMessages::unpack_message_struct<ZstMessages::RegisterEndpointAck>(responseMsg);

		//Connect performer dealer to stage now that it's been registered successfully
		char stage_router_addr[30];
		sprintf(stage_router_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_ROUTER_PORT);
		
		zsock_set_identity(m_stage_router, endpoint_ack.assigned_uuid.c_str());
		attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
		zsock_connect(m_stage_router, stage_router_addr);
	}
	else {
		throw runtime_error("PERFORMER: Stage performer registration responded with ERR");
	}
}

void Showtime::register_performer_to_stage(string performer){
	
}

chrono::milliseconds Showtime::ping_stage(){
	ZstMessages::Heartbeat beat;
    
    chrono::milliseconds delta = chrono::milliseconds(-1);
    chrono::time_point<chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    send_to_stage(ZstMessages::build_message<ZstMessages::Heartbeat>(ZstMessages::Kind::PERFORMER_HEARTBEAT, beat));

    //Get stage response
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);

    //Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type == ZstMessages::Kind::OK){
        end = chrono::system_clock::now();
        delta = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "PERFORMER: Client received heartbeat ping ack. Roundtrip was " <<  delta.count() << "ms" << endl;
    } else {
        throw runtime_error("PERFORMER: Stage ping responded with message other than OK");
    }

    return delta;
}

ZstPerformer * Showtime::create_performer(std::string name)
{
	ZstPerformer * perf = new ZstPerformer(name);
	Showtime::instance().m_performers[name] = perf;
	Showtime::instance().register_performer_to_stage(perf->get_name());
	return perf;
}

ZstPerformer * Showtime::get_performer(std::string performer)
{
	return Showtime::instance().m_performers[performer];
}

template ZST_EXPORT ZstIntPlug* Showtime::create_plug<ZstIntPlug>(std::string performer, std::string name, std::string instrument, PlugDir direction);
template ZST_EXPORT ZstFloatPlug* Showtime::create_plug<ZstFloatPlug>(std::string performer, std::string name, std::string instrument, PlugDir direction);
template ZST_EXPORT ZstIntArrayPlug* Showtime::create_plug<ZstIntArrayPlug>(std::string performer, std::string name, std::string instrument, PlugDir direction);
template ZST_EXPORT ZstFloatArrayPlug* Showtime::create_plug<ZstFloatArrayPlug>(std::string performer, std::string name, std::string instrument, PlugDir direction);
template ZST_EXPORT ZstStringPlug* Showtime::create_plug<ZstStringPlug>(std::string performer, std::string name, std::string instrument, PlugDir direction);
template<typename T>
T* Showtime::create_plug(std::string performer, std::string name, std::string instrument, PlugDir direction){
    
	ZstMessages::RegisterPlug plug_args;
	plug_args.performer = performer;
    plug_args.name = name;
    plug_args.instrument = instrument;
	plug_args.direction = direction;
    Showtime::instance().send_to_stage(ZstMessages::build_message<ZstMessages::RegisterPlug>(ZstMessages::Kind::STAGE_REGISTER_PLUG, plug_args));

    //Response
    zmsg_t *responseMsg = Showtime::instance().receive_from_stage();
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type != ZstMessages::Kind::OK){
        throw runtime_error("PERFORMER: Plug registration responded with message other than OK");
    }

    T *plug = new T(name, instrument, performer, direction);
	Showtime::instance().get_performer(performer)->add_plug(plug);
    return plug;
}


void Showtime::destroy_plug(ZstPlug * plug)
{
	ZstMessages::DestroyPlug plug_args;
	plug_args.address = plug->get_address();
	send_to_stage(ZstMessages::build_message<ZstMessages::DestroyPlug>(ZstMessages::Kind::STAGE_DESTROY_PLUG, plug_args));
	
    zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type != ZstMessages::Kind::OK) {
		throw runtime_error("PERFORMER: Plug deletion responded with message other than OK");
	}
	
	m_performers[plug->get_performer()]->remove_plug(plug);
	delete plug;
}


std::vector<PlugAddress> Showtime::get_all_plug_addresses(string performer, string instrument){
	ZstMessages::ListPlugs plug_args;
    plug_args.performer = performer;
    plug_args.instrument = instrument;
    send_to_stage(ZstMessages::build_message<ZstMessages::ListPlugs>(ZstMessages::Kind::STAGE_LIST_PLUGS, plug_args));

    zmsg_t *responseMsg = receive_from_stage();
    
	ZstMessages::ListPlugsAck plugResponse;
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type == ZstMessages::Kind::STAGE_LIST_PLUGS_ACK){
        plugResponse = ZstMessages::unpack_message_struct<ZstMessages::ListPlugsAck>(responseMsg);
        for(vector<PlugAddress>::iterator plugIter = plugResponse.plugs.begin(); plugIter != plugResponse.plugs.end(); ++plugIter ){
            cout << "PERFORMER: Remote plug: " << plugIter->performer << "/" << plugIter->instrument << "/" << plugIter->name << endl;
        }
    } else {
        throw runtime_error("PERFORMER: Plug registration responded with message other than STAGE_LIST_PLUGS_ACK");
    }
    cout << "PERFORMER: Total returned remote plugs: " << plugResponse.plugs.size() << endl;

    return plugResponse.plugs;
}


void Showtime::connect_plugs(PlugAddress a, PlugAddress b)
{
	ZstMessages::ConnectPlugs plug_args;
	plug_args.first = a;
	plug_args.second = b;
	Showtime::instance().send_to_stage(ZstMessages::build_message<ZstMessages::ConnectPlugs>(ZstMessages::Kind::STAGE_REGISTER_CONNECTION, plug_args));
    
    zmsg_t *responseMsg = Showtime::instance().receive_from_stage();
    ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if (message_type != ZstMessages::Kind::OK) {
        throw runtime_error("PERFORMER: Plug connect responded with message other than OK");
    }
}
