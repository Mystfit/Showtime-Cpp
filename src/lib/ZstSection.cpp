#include "ZstSection.h"
#include "ZstInstrument.h"


using namespace Showtime;

ZstSection::ZstSection(string name)
{
	m_name = name;

	string stage_addr = "tcp://127.0.0.1:6000";

	m_stage = zsock_new_req((">" + stage_addr).c_str());
	m_reply = zsock_new_rep("@tcp://*:*");
    
	m_graph_in = zsock_new_sub("", "");
	m_graph_out = zsock_new_pub("@tcp://*:*");

	m_loop = zloop_new();
    start_heartbeat();
}


ZstSection::~ZstSection()
{
}

void ZstSection::start_client(){
}

unique_ptr<ZstSection> ZstSection::create_section(string name)
{
	return unique_ptr<ZstSection>(new ZstSection(name));
}

shared_ptr<ZstInstrument> ZstSection::create_instrument(string name)
{
	shared_ptr<ZstInstrument> instrument = shared_ptr<ZstInstrument>( new ZstInstrument(name));
	m_instruments.push_back(instrument);

	return instrument;
}

void ZstSection::destroy_instrument(ZstInstrument& instrument)
{
}

vector<shared_ptr<ZstInstrument> >& ZstSection::get_instruments()
{
	return m_instruments;
}

void ZstSection::register_to_stage(){
    Messages::RegisterSection args;
    args.name = m_name;
    args.endpoint = "some_endpoint_name";
    zmsg_t * msg = Messages::build_register_section_message(args);
    zmsg_send(&msg, m_stage);
    
    //Wait for ack
}

void ZstSection::start_heartbeat(){
    zloop_timer (m_loop, 5000, 0, s_heartbeat_timer, this);
    zloop_start (m_loop);
}

int ZstSection::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((ZstSection*)arg)->send_heartbeat();
}

void ZstSection::send_heartbeat(){
    Messages::Heartbeat beat;
    auto now = chrono::system_clock::now().time_since_epoch();
    beat.from = m_name;
    beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    zmsg_t * msg = Messages::build_heartbeat_message(beat);
    zmsg_send(&msg, m_stage);
    cout << "Sent heartbeat" << endl;
    
    zmsg_t *responseMsg = zmsg_recv(m_stage);
    cout << "Client received heartbeat ack" << endl;
}


