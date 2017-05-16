#include "ZstStage.h"

using namespace std;
using namespace ZstMessages;

ZstStage::ZstStage()
{
	m_section_router = zsock_new_router("@tcp://*:6000");
    m_graph_update_pub = zsock_new_pub("@tcp://*:6001");
    
    m_loop = zloop_new();
    zloop_reader(m_loop, m_section_router, s_handle_router, this);
    m_loop_actor = zactor_new(actor_thread_func, this);
}

ZstStage::~ZstStage()
{
}


ZstStage* ZstStage::create_stage()
{
    ZstStage* stage = new ZstStage();
    return stage;
}

ZstPerformerRef ZstStage::get_performer_ref(string performer_name){
    map<string, ZstPerformerRef>::iterator performerRefIter = m_performer_refs.find(performer_name);
    
    if(performerRefIter == m_performer_refs.end())
        throw out_of_range("Could not find section");

    return performerRefIter->second;
}


void ZstStage::actor_thread_func(zsock_t *pipe, void *args)
{
    cout << "STAGE: Starting stage actor" << endl;
    zsock_signal (pipe, 0);

    ZstStage* stage = (ZstStage*)args;
    cout << "STAGE: Starting stage event loop" << endl;
    stage->start_server_event_loop();
    
    cout << "STAGE: Server exited" << endl;
}

void ZstStage::start_server_event_loop(){
    zloop_start(m_loop);
}

int ZstStage::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstStage *stage = (ZstStage*)arg;
    
    //Receive waiting message
	zmsg_t *msg = zmsg_recv(socket);
    
    //Get identity
    zframe_t *identity = zmsg_pop(msg);
    
    //Remove spacer
    zmsg_pop(msg);
    
    //Get message type
    MessageIds message_type = pop_message_id(msg);
    
    switch (message_type) {
        case MessageIds::STAGE_REGISTER_PERFORMER:
            stage->register_section_handler(socket, identity, msg);
            break;
        case MessageIds::PERFORMER_HEARTBEAT:
            stage->section_heartbeat_handler(socket, identity, msg);
            break;
        case MessageIds::STAGE_REGISTER_PLUG:
            stage->register_plug_handler(socket, identity, msg);
            break;
        case MessageIds::STAGE_LIST_PLUGS:
            stage->list_plugs_handler(socket, identity, msg);
            break;
        default:
            cout << "Didn't understand received message type of " << message_type << endl;
            break;
    }

	return 0;
}


int ZstStage::s_handle_section_pipe(zloop_t * loop, zsock_t * socket, void * arg)
{
    ZstStage *stage = (ZstStage*)arg;
    
    //Receive waiting message
    zmsg_t *msg = zmsg_recv(socket);
    
    //Get message type
    MessageIds message_type = pop_message_id(msg);
    
    cout << "STAGE: Message from section pipe" << endl;
    
    return 0;
}


void ZstStage::register_section_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg){
    RegisterPerformer section_args = unpack_message_struct<RegisterPerformer>(msg);
    
    if(m_performer_refs.count(section_args.name) > 0){
        cout << "STAGE: Section already registered!" << endl;
        return;
    }
    cout << "STAGE: Registering new section " << section_args.name << endl;

    //Create new pair socket to handle messages to/from to section
    ZstPerformerRef sectionRef;
    sectionRef.name = section_args.name;
    sectionRef.pipe = zsock_new_pair("@tcp://127.0.0.1:*");
    
    m_performer_refs[section_args.name] = sectionRef;
    char *last_endpoint = zsock_last_endpoint(sectionRef.pipe);
    
    //Register section pipe to poller
    zloop_reader(m_loop, sectionRef.pipe, s_handle_section_pipe, this);

    //Find endpoint port
    RegisterPerformerAck ack_args;
    std::cmatch matches;
    regex endpoint_expr("(?:tcp?)(?::\/\/)([^:]*):([0-9]{5})?(.*)");
    std::regex_match ( last_endpoint, matches, endpoint_expr, std::regex_constants::match_default );
    ack_args.assigned_stage_port = std::atoi(matches.str(2).c_str());
    cout << "STAGE: New section port: " << ack_args.assigned_stage_port << endl;
    
    zmsg_t *ackmsg = build_message<RegisterPerformerAck>(MessageIds::STAGE_REGISTER_PERFORMER_ACK, ack_args ,identity);
    zmsg_send(&ackmsg, socket);
    
    //Test new section pipe with a handshake
    zmsg_send(&ackmsg, sectionRef.pipe);
}

void ZstStage::register_plug_handler(zsock_t *socket, zframe_t *identity, zmsg_t *msg){
    RegisterPlug plug_args = unpack_message_struct<RegisterPlug>(msg);
    
    if(m_performer_refs.empty()){
       cout << "STAGE: Couldn't register plug. No section registered to stage with name";
    }
    
    cout << "STAGE: Registering new plug " << plug_args.name << endl;
    ZstPlugAddress plugRef;
    plugRef.name = plug_args.name;
    plugRef.instrument = plug_args.instrument;
    plugRef.performer = plug_args.performer;
    
    m_performer_refs[plug_args.performer].plugs.push_back(plugRef);
    
    zmsg_t *ackmsg = build_message<OKAck>(MessageIds::OK, OKAck() ,identity);
    zmsg_send(&ackmsg, socket);

}

void ZstStage::section_heartbeat_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg){
    Heartbeat heartbeat_args = unpack_message_struct<Heartbeat>(msg);
    cout << "STAGE: Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;
    
    zmsg_t *ackmsg = build_message(MessageIds::OK, NULL ,identity);
    zmsg_send(&ackmsg, socket);
}

void ZstStage::list_plugs_handler(zsock_t *socket, zframe_t *identity, zmsg_t *msg){
    ListPlugs plug_query_args = unpack_message_struct<ListPlugs>(msg);
    cout << "STAGE: Received list plugs request" << endl;
    
    ListPlugsAck response;
    
    //Filter plugs based on query args
    if(plug_query_args.performer.empty()){
        //Return all plugs
        for(map<string,ZstPerformerRef>::iterator performerIter = m_performer_refs.begin(); performerIter != m_performer_refs.end(); ++performerIter) {
            response.plugs.insert(response.plugs.end(), performerIter->second.plugs.begin(), performerIter->second.plugs.end());
        }
    } else {
        //Return plugs from named section
        vector<ZstPlugAddress> plugs = m_performer_refs[plug_query_args.performer].plugs;
        if(plug_query_args.instrument.empty()){
            //Return all plugs from section
            response.plugs = m_performer_refs[plug_query_args.performer].plugs;
        } else {
            //Return only named instrument plugs from section
            for(vector<ZstPlugAddress>::iterator plugIter = plugs.begin(); plugIter != plugs.end(); ++plugIter){
                if(plugIter->instrument == plug_query_args.instrument){
                    response.plugs.push_back(*plugIter);
                }
            }
        }
    }

    zmsg_t *ackmsg = build_message<ListPlugsAck>(MessageIds::STAGE_LIST_PLUGS_ACK, response, identity);
    zmsg_send(&ackmsg, socket);
}





