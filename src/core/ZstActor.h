#pragma once

#include <czmq.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <ZstExports.h>

class ZstActor {
public:
	ZST_EXPORT ZstActor();
	ZST_EXPORT virtual ~ZstActor();
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void start_loop();
	ZST_EXPORT virtual void stop_loop();
	ZST_EXPORT virtual bool is_running();
    ZST_EXPORT const char * actor_name() const;
	ZST_EXPORT virtual void process_callbacks() {};
	
	ZST_EXPORT int attach_timer(int delay, std::function<void()> timer_func);
	ZST_EXPORT void detach_timer(int timer_id);
	ZST_EXPORT void attach_pipe_listener(zsock_t* sock, zloop_reader_fn handler, void *args);

	ZST_EXPORT void self_test();

private:
    std::string m_actor_name;
    
	//Loop
	zloop_t * m_loop;
	bool m_is_running;
	void start_polling(zsock_t * pipe);

	//CZMQ Actor
	zactor_t *m_loop_actor;
	static void actor_thread_func(zsock_t *pipe, void *args);

	//Handlers
	static int s_handle_actor_pipe(zloop_t *loop, zsock_t *sock, void *arg);
	static int s_handle_timer(zloop_t * loop, int timer_id, void * arg);

	//Timers
	std::unordered_map<int, std::function<void()> > m_timers;
};
