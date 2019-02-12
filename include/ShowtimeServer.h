#pragma once

#include <ZstConstants.h>
#include <ZstExports.h>
#include <memory>
#include <string>

extern "C" {
	ZST_SERVER_EXPORT void * zst_create_server(const char * server_name, int port = STAGE_ROUTER_PORT);
	ZST_SERVER_EXPORT void zst_destroy_server(void * server);
}
