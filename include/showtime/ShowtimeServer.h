#pragma once

#include <memory>
#include <string>

#include "ZstConstants.h"
#include "ZstExports.h"

extern "C" {
	struct ServerHandle {
	public:
		void * server_ptr;
	};

	ZST_SERVER_EXPORT ServerHandle zst_create_server(const char * server_name, int port = STAGE_ROUTER_PORT);
	ZST_SERVER_EXPORT void zst_destroy_server(const ServerHandle & server);
}
