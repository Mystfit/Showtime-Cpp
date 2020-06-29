#include "ShowtimeServer.h"

void UShowtimeServer::BeginDestroy()
{
	Super::BeginDestroy();
	this->destroy();
}
