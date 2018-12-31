#include "ZstActor.h"
#include <ZstLogging.h>
#include <czmq.h>

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
	zloop_set_verbose(m_loop, true);
	zloop_set_nonstop(m_loop, true);
}

void ZstActor::start_loop()
{
	if (!m_loop) {
		ZstLog::net(LogLevel::error, "Init reactor before starting loop");
	}
	m_reactor_event_thread = boost::thread(boost::bind(&ZstActor::reactor_thread_func, this));
	m_is_running = true;
}

void ZstActor::stop_loop()
{
	zloop_destroy(&m_loop);
	m_loop = NULL;
	m_reactor_event_thread.interrupt();
	m_reactor_event_thread.try_join_for(boost::chrono::milliseconds(250));
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

void ZstActor::reactor_thread_func()
{
	zloop_start(m_loop);
}

void ZstActor::attach_pipe_listener(zsock_t * sock, zloop_reader_fn handler, void * args)
{
	zloop_reader(m_loop, sock, handler, args);
}

void ZstActor::deattach_pipe_listener(zsock_t * sock, zloop_reader_fn handler)
{
	if (m_loop && sock) {
		zloop_reader_end(m_loop, sock);
	}
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
