#include "ZstActor.h"

using namespace std;

ZstActor::ZstActor()
{
}

ZstActor::~ZstActor()
{
}

void ZstActor::destroy()
{
}

void ZstActor::init(const char * name)
{
	m_actor_name = std::string(name);
	m_loop = zloop_new();
	zloop_set_verbose(m_loop, false);
	zloop_set_nonstop(m_loop, true);
}

void ZstActor::start_loop()
{
	m_is_running = true;
	m_loop_actor = zactor_new(actor_thread_func, this);
}

void ZstActor::stop_loop()
{
	zactor_destroy(&m_loop_actor);
	m_loop_actor = NULL;
	m_is_running = false;
}

bool ZstActor::is_running()
{
	return m_is_running;
}

const char * ZstActor::name() const
{
	return m_actor_name.c_str();
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

void ZstActor::attach_pipe_listener(zsock_t * sock, zloop_reader_fn handler, void * args)
{
	zloop_reader(m_loop, sock, handler, args);
}

int ZstActor::attach_timer(int delay, std::function<void()> timer_func)
{
	int timer_id = zloop_timer(m_loop, delay, 0, ZstActor::s_handle_timer, this);
	m_timers[timer_id] = timer_func;
	return timer_id;
}

void ZstActor::detach_timer(int timer_id)
{
	if(is_running())
		zloop_timer_end(m_loop, timer_id);
}

int ZstActor::s_handle_timer(zloop_t * loop, int timer_id, void * arg)
{
	ZstActor * actor = (ZstActor*)arg;
	if (actor->m_timers.find(timer_id) != actor->m_timers.end()) {
		actor->m_timers[timer_id]();
	}
	return 0;
}
