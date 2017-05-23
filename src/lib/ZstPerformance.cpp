#include "ZstPerformance.h"

using namespace std;
using namespace ZstMessages;

ZstPerformance* ZstPerformance::create_performer(string performer_name){
    return new ZstPerformance(performer_name);
}

ZstPerformance::ZstPerformance(string name){
    m_performer_name = name;

	string stage_addr = "tcp://127.0.0.1";
    
    m_stage_requests = zsock_new_req((">" + stage_addr + ":" + std::to_string(m_stage_req_port)).c_str());
	m_graph_in = zsock_new_sub("", "");
	m_graph_out = zsock_new_pub("@tcp://*:*");

    m_loop = zloop_new();
    
    //Register graph input socket to poller
    zloop_reader(m_loop, m_graph_in, s_handle_graph_in, this);
}

ZstPerformance::~ZstPerformance(){
    
}

string ZstPerformance::get_performer_name(){
    return m_performer_name;
}

std::vector<ZstPlug*> ZstPerformance::get_all_plugs(){
    vector<ZstPlug*> plugs;
    
    for(map<string,vector<ZstPlug*>>::iterator instrumentIter = m_plugs.begin(); instrumentIter != m_plugs.end(); ++instrumentIter) {
        for(vector<ZstPlug*>::iterator plugIter = instrumentIter->second.begin(); plugIter != instrumentIter->second.end(); ++plugIter){
            plugs.push_back(*plugIter);
        }
    }
    return plugs;
}

std::vector<ZstPlug*> ZstPerformance::get_instrument_plugs(std::string instrument){
    return m_plugs[instrument];
}


void ZstPerformance::start(){
    //Create poller actor
    m_loop_actor = zactor_new(actor_thread_func, this);
    register_to_stage();
}

void ZstPerformance::actor_thread_func(zsock_t *pipe, void *args){
    cout << "Starting performer actor" << endl;
    
    //We need to signal the actor pipe to get things going
    zsock_signal (pipe, 0);
    
    ZstPerformance* section = (ZstPerformance*)args;
    section->start_client_event_loop();
    cout << "Performer exited" << endl;
}

void ZstPerformance::start_client_event_loop(){
    zloop_timer (m_loop, 5000, 0, s_heartbeat_timer, this);
    zloop_start (m_loop);
}

int ZstPerformance::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
	return 0;
}

int ZstPerformance::s_handle_stage_pipe(zloop_t * loop, zsock_t * socket, void * arg){
    ZstPerformance *section = (ZstPerformance*)arg;
    
    //Receive waiting message
    zmsg_t *msg = zmsg_recv(socket);
    
    //Get message type
    MessageIds message_type = pop_message_id(msg);
    
    cout << "Message from stage pipe" << endl;
    
    return 0;
}


int ZstPerformance::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((ZstPerformance*)arg)->ping_stage();
	return 0;
}

void ZstPerformance::register_to_stage(){
    RegisterPerformer args;
    args.name = m_performer_name;
    args.endpoint = "some_endpoint_name";
    zmsg_t * msg = build_message<RegisterPerformer>(MessageIds::STAGE_REGISTER_PERFORMER, args);
    zmsg_send(&msg, m_stage_requests);
    
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
    MessageIds message_type = pop_message_id(responseMsg);
    if(message_type == MessageIds::STAGE_REGISTER_PERFORMER_ACK){
        RegisterPerformerAck register_ack = unpack_message_struct<RegisterPerformerAck>(responseMsg);
        m_stage_pipe = zsock_new_pair((">tcp://127.0.0.1:" + std::to_string(register_ack.assigned_stage_port)).c_str());
        
        zloop_reader(m_loop, m_stage_pipe, s_handle_stage_pipe, this);
        
        cout << "Section successfully registered to stage. Port is " << register_ack.assigned_stage_port << endl;
    } else {
        throw runtime_error("Stage section registration responded with ERR");
    }
}

chrono::milliseconds ZstPerformance::ping_stage(){
    Heartbeat beat;
    
    chrono::milliseconds delta = chrono::milliseconds(-1);
    chrono::time_point<chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    beat.from = m_performer_name;
    beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    zmsg_t * msg = build_message<Heartbeat>(MessageIds::PERFORMER_HEARTBEAT, beat);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
    //Get message type
    MessageIds message_type = pop_message_id(responseMsg);
    if(message_type == MessageIds::OK){
        end = chrono::system_clock::now();
        delta = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "Client received heartbeat ping ack. Roundtrip was " <<  delta.count() << "ms" << endl;
    } else {
        throw runtime_error("Stage ping responded with message other than OK");
    }

    return delta;
}


ZstPlug* ZstPerformance::create_plug(std::string name, std::string instrument, ZstPlug::PlugDirection direction){
    
    RegisterPlug plug_args;
    plug_args.performer = m_performer_name;
    plug_args.name = name;
    plug_args.instrument = instrument;
    
    zmsg_t * msg = build_message<RegisterPlug>(MessageIds::STAGE_REGISTER_PLUG, plug_args);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
    MessageIds message_type = pop_message_id(responseMsg);
    if(message_type != MessageIds::OK){
        throw runtime_error("Plug registration responded with message other than OK");
    }

    ZstPlug *plug = new ZstPlug(name, instrument, direction);
    m_plugs[instrument].push_back(plug);
    return plug;
}

std::vector<ZstPlugAddress> ZstPerformance::get_plug_addresses(string performer, string instrument){
    ListPlugs plug_args;
    plug_args.performer = performer;
    plug_args.instrument = instrument;
    
    zmsg_t * msg = build_message<ListPlugs>(MessageIds::STAGE_LIST_PLUGS, plug_args);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
    ListPlugsAck plugResponse;
    
    MessageIds message_type = pop_message_id(responseMsg);
    if(message_type == MessageIds::STAGE_LIST_PLUGS_ACK){
        plugResponse = ZstMessages::unpack_message_struct<ListPlugsAck>(responseMsg);
        for(vector<ZstPlugAddress>::iterator plugIter = plugResponse.plugs.begin(); plugIter != plugResponse.plugs.end(); ++plugIter ){
            cout << "Remote plug: " << plugIter->performer << "/" << plugIter->instrument << "/" << plugIter->name << endl;
        }
    } else {
        throw runtime_error("Plug registration responded with message other than STAGE_LIST_PLUGS_ACK");
    }
    cout << "Total returned remote plugs: " << plugResponse.plugs.size() << endl;
    
    
    return plugResponse.plugs;
}
