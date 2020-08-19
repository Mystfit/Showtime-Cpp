#include "ShowtimeServer.h"
#include <showtime/ShowtimeServer.h>


void UShowtimeServer::BeginDestroy()
{
	Super::BeginDestroy();
	if(server) server->destroy();
}

TSharedPtr<showtime::ShowtimeServer>& UShowtimeServer::Handle()
{
	return server;
}
