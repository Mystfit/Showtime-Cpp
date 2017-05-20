#pragma once

#include <czmq.h>
#include <iostream>
#include "ZstExports.h"

class ZstActor {
public:
	ZstActor();
	~ZstActor();

	virtual void start();
	virtual void stop();

	ZST_EXPORT void self_test();

protected:
	void attach_pipe_listener(zsock_t* sock, zloop_reader_fn handler, void *args);

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