#include "ShowtimeServer.h"
#include <showtime/ShowtimeServer.h>

UShowtimeServer::UShowtimeServer() :
	server(MakeShared<ShowtimeServer>())
{
}

void UShowtimeServer::BeginDestroy()
{
	Super::BeginDestroy();
	if(server) server->destroy();
}

TSharedPtr<showtime::ShowtimeServer>& UShowtimeServer::Handle()
{
	return server;
}
