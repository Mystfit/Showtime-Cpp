#include <ShowtimeServer.h>
#include "ZstStage.h"

void * zst_create_server(const char * server_name, int port)
{
	auto stage = new ZstStage();
	stage->init_stage(server_name, port);
	return stage;
}

void zst_destroy_server(void * server)
{
	ZstStage * server_cast = (ZstStage*)server;
	delete server_cast;
}
