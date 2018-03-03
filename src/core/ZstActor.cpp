#include "ZstActor.h"
#include <czmq.h>

using namespace std;

ZstActor::ZstActor()
{
}

ZstActor::~ZstActor()
{
	destroy();
}

void ZstActor::destroy()
{
	stop();
}

void ZstActor::init(const char * actor_name)
{
	m_loop = zloop_new();
	zloop_set_verbose(m_loop, false);
	zloop_set_nonstop(m_loop, true);
    m_actor_name = std::string(actor_name);
}

void ZstActor::start()
{
	m_loop_actor = zactor_new(actor_thread_func, this);
}

void ZstActor::stop()
{
	zactor_destroy(&m_loop_actor);
	m_loop_actor = NULL;
}

void ZstActor::start_polling(zsock_t * pipe)
{
	zloop_reader(m_loop, pipe, s_handle_actor_pipe, this);
	zloop_start(m_loop);
}

void ZstActor::actor_thread_func(zsock_t * pipe, void * args)
{
	//We need to signal the actor pipe to get things going
	zsock_signal(pipe, 0);

	ZstActor* actor = (ZstActor*)args;
	actor->start_polling(pipe);
	
	zloop_t * loop = actor->m_loop;
	zloop_destroy(&loop);
	actor->m_loop = NULL;
}


int ZstActor::s_handle_actor_pipe(zloop_t * loop, zsock_t * sock, void * args)
{
	zmsg_t *msg = zmsg_recv(sock);

	//Received TERM message, this actor is going away
	char *command = zmsg_popstr(msg);
	if (streq(command, "$TERM")) {
		ZstActor * actor = (ZstActor*)args;
		
		//Signal that we finished cleaning up
		zsock_signal(sock, 0);

		//Return -1 to exit the zloop
		return -1;
	}
	else if (streq(command, "PING")) {
		zstr_send(sock, "PONG");
	}
	return 0;
}

void ZstActor::self_test()
{
	zmsg_t *msg = zmsg_new();
	zmsg_addstr(msg, "PING");
	zactor_send(m_loop_actor, &msg);
	zmsg_t * response = zactor_recv(m_loop_actor);
    char * response_s = zmsg_popstr(response);
	assert(streq(response_s, "PONG"));
    zstr_free(&response_s);
	zmsg_destroy(&response);
}

const char* ZstActor::actor_name() const {
    return m_actor_name.c_str();
}

void ZstActor::attach_pipe_listener(zsock_t * sock, zloop_reader_fn handler, void * args)
{
	zloop_reader(m_loop, sock, handler, args);
}

int ZstActor::attach_timer(zloop_timer_fn handler, int delay, void * args)
{
	return zloop_timer(m_loop, delay, 0, handler, args);
}

void ZstActor::detach_timer(int timer_id)
{
	zloop_timer_end(m_loop, timer_id);
}
