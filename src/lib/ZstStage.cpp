#include "ZstStage.h"

using namespace Showtime;

ZstStage::ZstStage()
{
	m_section_router = zsock_new_router("@tcp://*:6000");

    m_graph_update_pub = zsock_new_pub("@tcp://*:6001");
    m_loop = zloop_new();
    
    zloop_reader(m_loop, m_section_router, s_handle_router, this);

    m_loop_actor = zactor_new(thread_loop_func, this);
}

ZstStage::~ZstStage()
{
}


ZstStage* ZstStage::create_stage()
{
    ZstStage* stage = new ZstStage();
    return stage;
}


void ZstStage::thread_loop_func(zsock_t *pipe, void *args)
{
    zsock_signal (pipe, 0);

    cout << "Starting stage actor" << endl;

    ZstStage* stage = (ZstStage*)args;
    cout << "Starting stage event loop" << endl;
    stage->start_server();
    
    cout << "Server exited" << endl;
}

void ZstStage::start_server(){
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
    char * msg_type_str = zmsg_popstr(msg);
    int converted_msg_id = atoi(msg_type_str);
    Messages::MessageIds message_type = (Messages::MessageIds)converted_msg_id;
        
    switch (message_type) {
        case Messages::MessageIds::STAGE_REGISTER_SECTION:
            stage->register_section_handler(msg);
            break;
        case Messages::MessageIds::SECTION_HEARTBEAT:
            stage->section_heartbeat_handler(msg);
            stage->send_section_heartbeat_ack(socket, identity);
            break;
        default:
            cout << "Didn't understand received message type of " << message_type << endl;
            break;
    }

	return 0;
}

void ZstStage::register_section_handler(zmsg_t * msg){
    Messages::RegisterSection section_args = Messages::unpack_message_struct<Messages::RegisterSection>(msg);
    cout << "Registering new section " << section_args.name << endl;
    m_section_endpoints.push_back(tuple<string, string>(section_args.name, section_args.endpoint));
}

void ZstStage::section_heartbeat_handler(zmsg_t * msg){
    Messages::Heartbeat heartbeat_args = Messages::unpack_message_struct<Messages::Heartbeat>(msg);
    cout << "Received heartbeat from " << heartbeat_args.from << ". Timestamp: " << heartbeat_args.timestamp << endl;
}

void ZstStage::send_section_heartbeat_ack(zsock_t * socket, zframe_t * identity){
    zmsg_t *responseMsg = zmsg_new();
    zmsg_add(responseMsg, identity);
    zmsg_add(responseMsg, zframe_new_empty());
    zmsg_add(responseMsg, Messages::build_message_id_frame(Messages::MessageIds::OK));
    zmsg_send(&responseMsg, socket);
    cout << "Sending heartbeat ack" << endl;
}




