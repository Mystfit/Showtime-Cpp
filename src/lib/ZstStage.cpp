#include "ZstStage.h"

using namespace Showtime;

ZstStage::ZstStage()
{
	m_section_router = zsock_new_router("@tcp://*:6000");

    m_graph_update_pub = zsock_new_pub("@tcp://*:6001");
    m_poller = zpoller_new (m_section_router, NULL);
    m_loop = zloop_new();
    
    zloop_reader(m_loop, m_section_router, s_handle_router, NULL);
    

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
    stage->start();

//    while(true){
//        cout << "Loop" << endl;
//        stage->event_loop();
//    }
    
    cout << "Loop exit" << endl;
}

void ZstStage::event_loop(){

//    zsock_t *sock = (zsock_t*)zpoller_wait (m_poller, -1);
//    assert(sock != NULL);
//    cout << "Received: " << zstr_recv(sock) << endl;
//    cout << "boop" << endl;
}

void ZstStage::start(){
    zloop_start(m_loop);
}

int ZstStage::s_handle_router(zloop_t * loop, zsock_t * socket, void * arg)
{
	zmsg_t *msg = zmsg_recv(socket);
    
    cout << "Server received message of " << zmsg_size(msg) << " frames" << endl;

    zframe_t *identity = zmsg_pop(msg);
    zframe_print(identity, "Identity was ");
    
    zmsg_pop(msg);
    string clientGreeting = zmsg_popstr(msg);
    
    cout << "Greeting was " << clientGreeting << endl;
    
    zmsg_t *responseMsg = zmsg_new();
    zmsg_add(responseMsg, identity);
    zmsg_add(responseMsg, zframe_new_empty());
    zmsg_addstr(responseMsg, "pong");
    
    cout << "Responding with pong" << endl;
    
    zmsg_send(&responseMsg, socket);
    
    cout << "Sent" << endl;

	return 0;
}
