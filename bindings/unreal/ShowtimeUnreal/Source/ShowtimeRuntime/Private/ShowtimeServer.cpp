#include "ShowtimeServer.h"

void UShowtimeServer::BeginDestroy()
{
	Super::BeginDestroy();
	server->destroy();
}

TSharedPtr<ShowtimeServer>& UShowtimeServer::Handle()
{
	return server;
}
