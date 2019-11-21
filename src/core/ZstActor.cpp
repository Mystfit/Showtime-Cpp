#include "ZstActor.h"
#include "ZstZMQRefCounter.h"
#include "ZstLogging.h"
#include <czmq.h>

using namespace std;

namespace showtime {

	ZstActor::ZstActor() :
		m_loop(NULL),
		m_loop_actor(NULL)
	{
	}

	ZstActor::~ZstActor()
	{
		stop_loop();
	}

	void ZstActor::destroy()
	{
	}

	void ZstActor::init(const char* name)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		m_actor_name = std::string(name);
		m_loop = zloop_new();
		//zloop_set_verbose(m_loop, true);
		zloop_set_nonstop(m_loop, true);
		zst_zmq_inc_ref_count();
	}

	void ZstActor::start_loop()
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		m_is_running = true;
		m_loop_actor = zactor_new(actor_thread_func, this);
		//m_loop_thread = boost::thread(boost::bind(&ZstActor::actor_loop_func, this));
	}

	void ZstActor::stop_loop()
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		if (m_is_running) {
			if (m_loop_actor) {
				zactor_destroy(&m_loop_actor);
				zst_zmq_dec_ref_count();
			}

			m_loop = NULL;
			m_loop_actor = NULL;
			m_is_running = false;
		}
	}

	bool ZstActor::is_running()
	{
		return m_is_running;
	}

	const char* ZstActor::name() const
	{
		return m_actor_name.c_str();
	}

	void ZstActor::actor_thread_func(zsock_t* pipe, void* args)
	{
		//We need to signal the actor pipe to get things going
		zsock_signal(pipe, 0);

		ZstActor* actor = (ZstActor*)args;

		//Register actor to the loop it controls so it can respond to incoming messages
		zloop_reader(actor->m_loop, pipe, s_handle_actor_pipe, actor);

		//Blocking loop start
		zloop_start(actor->m_loop);

		//Cleanup when loop returns
		zloop_destroy(&actor->m_loop);
	}

	void ZstActor::actor_loop_func()
	{
		////Blocking loop start
		//zloop_start(m_loop);

		////Cleanup when loop returns
		//zloop_destroy(&m_loop);
	}

	int ZstActor::s_handle_actor_pipe(zloop_t* loop, zsock_t* sock, void* args)
	{
		//zmsg_t *msg = zmsg_recv(sock);

		//Received TERM message, this actor is going away
		//char *command = zmsg_popstr(msg);
		int status = 0;
		char* command = "";
		zsock_t* send_sock = NULL;
		zmsg_t* msg = NULL;
		zsock_recv(sock, "spm", &command, &send_sock, &msg);

		if (streq(command, "$TERM")) {
			//Signal that we finished cleaning up
			zsock_signal(sock, 0);

			//Return -1 to exit the zloop
			status = -1;
		}
		else if (streq(command, "s")) {
			if (!send_sock) {
				ZstLog::net(LogLevel::error, "Actor received send request but no socket was sent");
			}
			if (!msg) {
				ZstLog::net(LogLevel::error, "Actor received send request but no msg was sent");
			}
			zmsg_send(&msg, send_sock);
		}
		else if (streq(command, "PING")) {
			zstr_send(sock, "PONG");
		}
		else {
			ZstLog::net(LogLevel::error, "Actor command not recognized: {}", command);
		}

		zstr_free(&command);
		return status;
	}

	void ZstActor::self_test()
	{
		zmsg_t* msg = zmsg_new();
		zmsg_addstr(msg, "PING");
		zactor_send(m_loop_actor, &msg);
		zmsg_t* response = zactor_recv(m_loop_actor);
		char* response_s = zmsg_popstr(response);
		assert(streq(response_s, "PONG"));
		zstr_free(&response_s);
		zmsg_destroy(&response);
	}

	void ZstActor::attach_pipe_listener(zsock_t* sock, zloop_reader_fn handler, void* args)
	{
		zloop_reader(m_loop, sock, handler, args);
	}

	void ZstActor::remove_pipe_listener(zsock_t* sock)
	{
		if(m_loop && sock)
			zloop_reader_end(m_loop, sock);
	}

	int ZstActor::send_to_socket(zsock_t* sock, zmsg_t* msg) const
	{
		int result = -1;
		if (m_loop_actor)
			result = zsock_send(m_loop_actor, "spm", "s", sock, msg);
		return result;
	}

	int ZstActor::attach_timer(int delay, std::function<void()> timer_func)
	{
		std::lock_guard<std::mutex> lock(m_mtx);
		int timer_id = zloop_timer(m_loop, delay, 0, ZstActor::s_handle_timer, this);
		m_timers[timer_id] = timer_func;
		return timer_id;
	}

	void ZstActor::detach_timer(int timer_id)
	{
		if (is_running())
			zloop_timer_end(m_loop, timer_id);
	}

	int ZstActor::s_handle_timer(zloop_t* loop, int timer_id, void* arg)
	{
		ZstActor* actor = (ZstActor*)arg;
		if (actor->m_timers.find(timer_id) != actor->m_timers.end()) {
			actor->m_timers[timer_id]();
		}
		return 0;
	}
}