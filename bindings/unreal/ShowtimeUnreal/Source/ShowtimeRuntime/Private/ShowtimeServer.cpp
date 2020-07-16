#include "ShowtimeServer.h"

void UShowtimeServer::BeginDestroy()
{
	Super::BeginDestroy();
	if(server) server->destroy();
}

TSharedPtr<ShowtimeServer>& UShowtimeServer::Handle()
{
	return server;
}
