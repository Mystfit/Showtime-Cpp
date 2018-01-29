#pragma once

#include <czmq.h>
#include <iostream>
#include <ZstExports.h>

class ZstActor {
public:
	ZST_EXPORT ZstActor();
	ZST_EXPORT virtual ~ZstActor();
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void start();
	ZST_EXPORT virtual void stop();
	ZST_EXPORT void self_test();

protected:
	ZST_EXPORT void attach_pipe_listener(zsock_t* sock, zloop_reader_fn handler, void *args);
	ZST_EXPORT int attach_timer(zloop_timer_fn handler, int delay, void *args);
	ZST_EXPORT void detach_timer(int timer_id);

private:
	//Loop
	zloop_t * m_loop;
	void start_polling(zsock_t * pipe);

	//CZMQ Actor
	zactor_t *m_loop_actor;
	static void actor_thread_func(zsock_t *pipe, void *args);

	//Handlers
	static int s_handle_actor_pipe(zloop_t *loop, zsock_t *sock, void *arg);
};
