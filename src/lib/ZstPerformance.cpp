#include "ZstPerformance.h"

using namespace std;

ZstPerformance* ZstPerformance::create_performer(string performer_name){
    return new ZstPerformance(performer_name);
}

ZstPerformance::ZstPerformance(string name){
    m_performer_name = name;

	string stage_addr = "tcp://127.0.0.1";
    
    m_stage_requests = zsock_new_req((">" + stage_addr + ":" + std::to_string(m_stage_req_port)).c_str());
	zsock_set_linger(m_stage_requests, 0);

	m_graph_in = zsock_new_sub("", "");
	zsock_set_linger(m_graph_in, 0);

	m_graph_out = zsock_new_pub("@tcp://*:*");
	zsock_set_linger(m_graph_out, 0);
    
	start();
}

ZstPerformance::~ZstPerformance(){
	ZstActor::~ZstActor();
	zsock_destroy(&m_stage_requests);
	zsock_destroy(&m_graph_in);
	zsock_destroy(&m_graph_out);
	if (m_stage_pipe) {
		zsock_destroy(&m_stage_pipe);
	}
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
	ZstActor::start();
    register_to_stage();
}

void ZstPerformance::stop() {
	ZstActor::stop();
}


int ZstPerformance::s_handle_graph_in(zloop_t * loop, zsock_t * socket, void * arg){
	return 0;
}

int ZstPerformance::s_handle_stage_pipe(zloop_t * loop, zsock_t * socket, void * arg){
    ZstPerformance *section = (ZstPerformance*)arg;
    
    //Receive waiting message
    zmsg_t *msg = zmsg_recv(socket);
    
    //Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(msg);
    
    cout << "Message from stage pipe" << endl;
    
    return 0;
}

int ZstPerformance::s_heartbeat_timer(zloop_t * loop, int timer_id, void * arg){
    ((ZstPerformance*)arg)->ping_stage();
	return 0;
}

void ZstPerformance::register_to_stage(){
	ZstMessages::RegisterPerformer args;
    args.name = m_performer_name;
    args.endpoint = "some_endpoint_name";
    zmsg_t * msg = ZstMessages::build_message<ZstMessages::RegisterPerformer>(ZstMessages::Kind::STAGE_REGISTER_PERFORMER, args);
    zmsg_send(&msg, m_stage_requests);
    
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type == ZstMessages::Kind::STAGE_REGISTER_PERFORMER_ACK){
		ZstMessages::RegisterPerformerAck register_ack = ZstMessages::unpack_message_struct<ZstMessages::RegisterPerformerAck>(responseMsg);
        m_stage_pipe = zsock_new_pair((">tcp://127.0.0.1:" + std::to_string(register_ack.assigned_stage_port)).c_str());
        
		//Attach pipe handler to loop
		attach_pipe_listener(m_stage_pipe, s_handle_stage_pipe, this);
        
        cout << "Section successfully registered to stage. Port is " << register_ack.assigned_stage_port << endl;
    } else {
        throw runtime_error("Stage section registration responded with ERR");
    }
}

chrono::milliseconds ZstPerformance::ping_stage(){
	ZstMessages::Heartbeat beat;
    
    chrono::milliseconds delta = chrono::milliseconds(-1);
    chrono::time_point<chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();
    
    beat.from = m_performer_name;
    beat.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    zmsg_t * msg = ZstMessages::build_message<ZstMessages::Heartbeat>(ZstMessages::Kind::PERFORMER_HEARTBEAT, beat);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
    //Get message type
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type == ZstMessages::Kind::OK){
        end = chrono::system_clock::now();
        delta = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "Client received heartbeat ping ack. Roundtrip was " <<  delta.count() << "ms" << endl;
    } else {
        throw runtime_error("Stage ping responded with message other than OK");
    }

    return delta;
}


ZstPlug* ZstPerformance::create_plug(std::string name, std::string instrument, ZstPlug::Direction direction){
    
	ZstMessages::RegisterPlug plug_args;
    plug_args.performer = m_performer_name;
    plug_args.name = name;
    plug_args.instrument = instrument;
    
    zmsg_t * msg = ZstMessages::build_message<ZstMessages::RegisterPlug>(ZstMessages::Kind::STAGE_REGISTER_PLUG, plug_args);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type != ZstMessages::Kind::OK){
        throw runtime_error("Plug registration responded with message other than OK");
    }

    ZstPlug *plug = new ZstPlug(name, instrument, m_performer_name, direction);
    m_plugs[instrument].push_back(plug);
    return plug;
}

void ZstPerformance::destroy_plug(ZstPlug * plug)
{
	ZstMessages::DestroyPlug plug_args;
	plug_args.address = plug->get_address();

	zmsg_t * msg = ZstMessages::build_message<ZstMessages::DestroyPlug>(ZstMessages::Kind::STAGE_DESTROY_PLUG, plug_args);
	zmsg_send(&msg, m_stage_requests);
	
	zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
	if (message_type != ZstMessages::Kind::OK) {
		throw runtime_error("Plug deletion responded with message other than OK");
	}
	
	m_plugs[plug->get_instrument()].erase(std::remove(m_plugs[plug->get_instrument()].begin(), m_plugs[plug->get_instrument()].end(), plug), m_plugs[plug->get_instrument()].end());
	delete plug;
}

std::vector<ZstPlug::Address> ZstPerformance::get_all_plug_addresses(string performer, string instrument){
	ZstMessages::ListPlugs plug_args;
    plug_args.performer = performer;
    plug_args.instrument = instrument;
    
    zmsg_t * msg = ZstMessages::build_message<ZstMessages::ListPlugs>(ZstMessages::Kind::STAGE_LIST_PLUGS, plug_args);
    zmsg_send(&msg, m_stage_requests);
    zmsg_t *responseMsg = zmsg_recv(m_stage_requests);
    
	ZstMessages::ListPlugsAck plugResponse;
    
	ZstMessages::Kind message_type = ZstMessages::pop_message_kind_frame(responseMsg);
    if(message_type == ZstMessages::Kind::STAGE_LIST_PLUGS_ACK){
        plugResponse = ZstMessages::unpack_message_struct<ZstMessages::ListPlugsAck>(responseMsg);
        for(vector<ZstPlug::Address>::iterator plugIter = plugResponse.plugs.begin(); plugIter != plugResponse.plugs.end(); ++plugIter ){
            cout << "Remote plug: " << plugIter->performer << "/" << plugIter->instrument << "/" << plugIter->name << endl;
        }
    } else {
        throw runtime_error("Plug registration responded with message other than STAGE_LIST_PLUGS_ACK");
    }
    cout << "Total returned remote plugs: " << plugResponse.plugs.size() << endl;

    return plugResponse.plugs;
}