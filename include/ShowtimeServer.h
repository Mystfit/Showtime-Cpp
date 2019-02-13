#pragma once

#include <ZstConstants.h>
#include <ZstExports.h>
#include <memory>
#include <string>

extern "C" {
	struct ServerHandle {
	public:
		void * server_ptr;
	};

	ZST_SERVER_EXPORT ServerHandle zst_create_server(const char * server_name, int port = STAGE_ROUTER_PORT);
	ZST_SERVER_EXPORT void zst_destroy_server(const ServerHandle & server);
}
