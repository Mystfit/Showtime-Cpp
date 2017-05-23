#include "Showtime.h"

using namespace std;

Showtime::Showtime(){
}

Showtime::~Showtime(){
    ZstActor::~ZstActor();
    zsock_destroy(&m_stage_requests);
    zsock_destroy(&m_stage_router);
    zsock_destroy(&m_graph_in);
    zsock_destroy(&m_graph_out);
}

void Showtime::join(string stage_address, string performer_name){
    m_performer_name = performer_name;
    m_stage_addr = stage_address;
    
    //Build endpoint addresses
    char stage_req_addr[30];
    sprintf(stage_req_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_REP_PORT);
    
    //Stage request socket for querying the performance
    m_stage_requests = zsock_new_req(stage_req_addr);
    zsock_set_linger(m_stage_requests, 0);
    
    //Local dealer socket for receiving messages forwarded from other performers
    m_stage_router = zsock_new(ZMQ_DEALER);
    zsock_set_identity(m_stage_router, m_performer_name.c_str());
    attach_pipe_listener(m_stage_router, s_handle_stage_router, this);
    
    //Connect performer dealer to stage now that it's been registered successfully
    char stage_router_addr[30];
    sprintf(stage_router_addr, "tcp://%s:%d", m_stage_addr.c_str(), STAGE_ROUTER_PORT);
    zsock_connect(m_stage_router, stage_router_addr);
    
    //Graph input socket
    m_graph_in = zsock_new(ZMQ_SUB);
    zsock_set_linger(m_graph_in, 0);
    attach_pipe_listener(m_graph_in, s_handle_graph_in, this);
    
    //Graph output socket
    m_graph_out = zsock_new_pub("@tcp://*:*");
    m_output_endpoint = zsock_last_endpoint(m_graph_out);
    zsock_set_linger(m_graph_out, 0);
    
    start();
}





// ---------------
// Local accessors
// ---------------

string Showtime::get_performer_name(){
    return m_performer_name;
}

std::vector<ZstPlug*> Showtime::get_all_plugs(){
    vector<ZstPlug*> plugs;
    
    for(map<string,vector<ZstPlug*>>::iterator instrumentIter = m_plugs.begin(); instrumentIter != m_plugs.end(); ++instrumentIter) {
        for(vector<ZstPlug*>::iterator plugIter = instrumentIter->second.begin(); plugIter != instrumentIter->second.end(); ++plugIter){
            plugs.push_back(*plugIter);
        }
    }
    return plugs;
}

std::vector<ZstPlug*> Showtime::get_instrument_plugs(std::string instrument){
    return m_plugs[instrument];
}


void Showtime::start(){
	ZstActor::start();
    register_to_stage();
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
    cout << "OMG I GOT A MESSAGE!!!" << endl;
    Showtime *performer = (Showtime*)arg;
    
    zmsg_t *msg = performer->receive_from_graph();
    string sender = zmsg_popstr(msg);
    
	//Do stuff with message args

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
    cout << "PERFORMER: " << m_performer_name << " connecting to " << performer_args.endpoint << ". My output endpoint is " << m_output_endpoint << endl;
}



// -------------
// API Functions
// -------------

void Showtime::register_to_stage(){
	ZstMessages::RegisterPerformer args;
    args.name = m_performer_name;
    args.endpoint = m_output_endpoint;
    cout << "PERFORMER: Registering performer" << endl;
    send_to_stage(ZstMessages::build_message<ZstMessages::RegisterPerformer>(ZstMessages::Kind::STAGE_REGISTER_PERFORMER, args));
    
    zmsg_t *responseMsg = receive_from_stage();
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type == ZstMessages::Kind::OK){
        cout << "PERFORMER: Successfully registered to stage." << endl;


    } else {
        throw runtime_error("PERFORMER: Stage performer registration responded with ERR");
    }
}

chrono::milliseconds Showtime::ping_stage(){
	ZstMessages::Heartbeat beat;
    
    chrono::milliseconds delta = chrono::milliseconds(-1);
    chrono::time_point<chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    beat.from = m_performer_name;
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


ZstPlug* Showtime::create_plug(std::string name, std::string instrument, PlugDir direction){
    
	ZstMessages::RegisterPlug plug_args;
	plug_args.performer = get_performer_name();
    plug_args.name = name;
    plug_args.instrument = instrument;
	plug_args.direction = direction;
    send_to_stage(ZstMessages::build_message<ZstMessages::RegisterPlug>(ZstMessages::Kind::STAGE_REGISTER_PLUG, plug_args));

    //Response
    zmsg_t *responseMsg = receive_from_stage();
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type != ZstMessages::Kind::OK){
        throw runtime_error("PERFORMER: Plug registration responded with message other than OK");
    }

    ZstPlug *plug = new ZstPlug(name, instrument, m_performer_name, direction);
    m_plugs[instrument].push_back(plug);
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
	
	m_plugs[plug->get_instrument()].erase(std::remove(m_plugs[plug->get_instrument()].begin(), m_plugs[plug->get_instrument()].end(), plug), m_plugs[plug->get_instrument()].end());
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
	send_to_stage(ZstMessages::build_message<ZstMessages::ConnectPlugs>(ZstMessages::Kind::STAGE_REGISTER_CONNECTION, plug_args));
    
    zmsg_t *responseMsg = receive_from_stage();
    ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if (message_type != ZstMessages::Kind::OK) {
        throw runtime_error("PERFORMER: Plug connect responded with message other than OK");
    }
}



