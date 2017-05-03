#include "ZstSection.h"
#include "ZstInstrument.h"

using namespace std;

ZstSection* ZstSection::create_section(string name){
    return new ZstSection(name);
}

ZstSection::~ZstSection(){
    
}

ZstSection::ZstSection(string name){
    m_name = name;

	string stage_addr = "tcp://127.0.0.1:6000";

	m_stage = zsock_new_req((">" + stage_addr).c_str());
	m_reply = zsock_new_rep("@tcp://*:*");
    
	m_graph_in = zsock_new_sub("", "");
	m_graph_out = zsock_new_pub("@tcp://*:*");

	m_loop = zloop_new();
    start_client();
}

void ZstSection::start_client(){
    start_heartbeat();
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
	return m_instruments;
}

void ZstSection::register_to_stage(){
    ZstMessages::RegisterSection args;
    args.name = m_name;
    args.endpoint = "some_endpoint_name";
    zmsg_t * msg = ZstMessages::build_register_section_message(args);
    zmsg_send(&msg, m_stage);
    
    //Wait for ack
}

void ZstSection::start_heartbeat(){
    zloop_timer (m_loop, 5000, 0, s_heartbeat_timer, this);
    zloop_start (m_loop);
}

int ZstSection::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((ZstSection*)arg)->ping_stage();
}

chrono::milliseconds ZstSection::ping_stage(){
    ZstMessages::Heartbeat beat;
    
    chrono::time_point<chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    beat.from = m_name;
    beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    zmsg_t * msg = ZstMessages::build_heartbeat_message(beat);
    zmsg_send(&msg, m_stage);
    cout << "Sent heartbeat ping" << endl;
    
    zmsg_t *responseMsg = zmsg_recv(m_stage);
    
    end = chrono::system_clock::now();
    
    chrono::milliseconds delta = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "Client received heartbeat ping ack. Roundtrip was " <<  delta.count() << "ms" << endl;
    return delta;
}


