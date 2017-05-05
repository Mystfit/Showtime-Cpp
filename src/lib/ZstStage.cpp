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


void ZstStage::actor_thread_func(zsock_t *pipe, void *args)
{
    cout << "Starting stage actor" << endl;
    zsock_signal (pipe, 0);

    ZstStage* stage = (ZstStage*)args;
    cout << "Starting stage event loop" << endl;
    stage->start_server_event_loop();
    
    cout << "Server exited" << endl;
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
        case MessageIds::STAGE_REGISTER_SECTION:
            stage->register_section_handler(socket, identity, msg);
            break;
        case MessageIds::SECTION_HEARTBEAT:
            stage->section_heartbeat_handler(socket, identity, msg);
            
            break;
        default:
            cout << "Didn't understand received message type of " << message_type << endl;
            break;
    }

	return 0;
}

void ZstStage::register_section_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg){
    RegisterSection section_args = unpack_message_struct<RegisterSection>(msg);
    cout << "Registering new section " << section_args.name << endl;
    
    zsock_t * section_pipe = zsock_new_pair("@tcp://127.0.0.1:*");
    m_section_pipes[section_args.name] = section_pipe;
    char *last_endpoint = zsock_last_endpoint(section_pipe);

    //Find endpoint port
    RegisterSectionAck ack_args;
    std::cmatch matches;
    regex endpoint_expr("(?:tcp?)(?::\/\/)([^:]*):([0-9]{5})?(.*)");
    std::regex_match ( last_endpoint, matches, endpoint_expr, std::regex_constants::match_default );
    ack_args.assigned_stage_port = std::atoi(matches.str(2).c_str());
    cout << "Sending port " << ack_args.assigned_stage_port << endl;
    
    zmsg_t *ackmsg = build_message<RegisterSectionAck>(MessageIds::STAGE_REGISTER_SECTION_ACK, ack_args ,identity);
    zmsg_send(&ackmsg, socket);
}


void ZstStage::section_heartbeat_handler(zsock_t * socket, zframe_t * identity, zmsg_t * msg){
    Heartbeat heartbeat_args = unpack_message_struct<Heartbeat>(msg);
    cout << "Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;
    
    zmsg_t *ackmsg = build_message(MessageIds::OK, NULL ,identity);
    zmsg_send(&ackmsg, socket);
}






