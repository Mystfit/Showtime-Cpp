#include <ShowtimeServer.h>
#include "ZstStage.h"

ServerHandle zst_create_server(const char * server_name, int port)
{
	auto stage = new ZstStage();
	stage->init_stage(server_name, port);
	return ServerHandle{ stage };
}

void zst_destroy_server(const ServerHandle & server)
{
	ZstStage * server_cast = (ZstStage*)server.server_ptr;
	delete server_cast;
}
