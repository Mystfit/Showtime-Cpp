#include "ZstSection.h"
#include "ZstInstrument.h"

using namespace std;
using namespace ZstMessages;

ZstSection* ZstSection::create_section(string name){
    return new ZstSection(name);
}

ZstSection::ZstSection(string name){
    m_name = name;

	string stage_addr = "tcp://127.0.0.1";
    
    m_stage_requests = zsock_new_req((">" + stage_addr + ":" + std::to_string(m_stage_req_port)).c_str());
    
	m_graph_in = zsock_new_sub("", "");
	m_graph_out = zsock_new_pub("@tcp://*:*");

    m_loop = zloop_new();
    zloop_reader(m_loop, m_graph_in, s_handle_graph_in, this);
    zloop_reader(m_loop, m_graph_in, s_handle_reply, this);

    m_loop_actor = zactor_new(actor_thread_func, this);
}

ZstSection::~ZstSection(){
    
}

void ZstSection::actor_thread_func(zsock_t *pipe, void *args){
    cout << "Starting section actor" << endl;
    
    //We need to signal the actor pipe to get things going
    zsock_signal (pipe, 0);
    
    ZstSection* section = (ZstSection*)args;
    section->start_client_event_loop();
    cout << "Section exited" << endl;
}

void ZstSection::start_client_event_loop(){
    zloop_timer (m_loop, 5000, 0, s_heartbeat_timer, this);
    zloop_start (m_loop);
}

int ZstSection::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
    
}

int ZstSection::s_handle_reply(zloop_t * loop, zsock_t * socket, void * arg){
    
}


ZstInstrument* ZstSection::create_instrument(string name)
{
	ZstInstrument* instrument = new ZstInstrument(name);
	m_instruments.push_back(instrument);

	return instrument;
}

void ZstSection::destroy_instrument(ZstInstrument& instrument)
{
}

vector<ZstInstrument*>& ZstSection::get_instruments()
{
    //query stage for instruments
	return m_instruments;
}

int ZstSection::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((ZstSection*)arg)->ping_stage();
}

chrono::milliseconds ZstSection::ping_stage(){
    Heartbeat beat;
    
    chrono::milliseconds delta = chrono::milliseconds(-1);
    chrono::time_point<chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    beat.from = m_name;
    beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    zmsg_t * msg = build_message<Heartbeat>(MessageIds::SECTION_HEARTBEAT, beat);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
    //Get message type
    MessageIds message_type = pop_message_id(responseMsg);
    if(message_type == MessageIds::OK){
        end = chrono::system_clock::now();
        delta = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "Client received heartbeat ping ack. Roundtrip was " <<  delta.count() << "ms" << endl;
    } else {
        throw runtime_error("Stage ping responded with ERR");
    }

    return delta;
}

void ZstSection::register_to_stage(){
    RegisterSection args;
    args.name = m_name;
    args.endpoint = "some_endpoint_name";
    zmsg_t * msg = build_message<RegisterSection>(MessageIds::STAGE_REGISTER_SECTION, args);
    zmsg_send(&msg, m_stage_requests);
    
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);

    MessageIds message_type = pop_message_id(responseMsg);
    if(message_type == MessageIds::STAGE_REGISTER_SECTION_ACK){
        zmsg_print(responseMsg);
        RegisterSectionAck register_ack = unpack_message_struct<RegisterSectionAck>(responseMsg);
        
        cout << "Section successfully registered to stage. Port is" << register_ack.assigned_stage_port << endl;
    } else {
        throw runtime_error("Stage section registration responded with ERR");
    }
    
    //TODO:Bind stage pipe to the unique endpoint returned from the stage
}


