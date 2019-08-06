#include <ShowtimeServer.h>
#include "ZstLogging.h"
#include "ZstStage.h"

ServerHandle zst_create_server(const char * server_name, int port)
{
	if(strlen(server_name) < 1){
		ZstLog::server(LogLevel::error, "Server name is empty");
		return ServerHandle{ NULL };
	}

	auto stage = new ZstStage();
	stage->init_stage(server_name, port);
	return ServerHandle{ stage };
}

void zst_destroy_server(const ServerHandle & server)
{
	ZstStage * server_cast = (ZstStage*)server.server_ptr;
	server_cast->destroy();
	delete server_cast;
}
