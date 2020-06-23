#include "ShowtimeServer.h"

UShowtimeServer::UShowtimeServer() : UObject(), ShowtimeServer() {
	this->init();
}

UShowtimeServer::UShowtimeServer(const FString& name) : UObject(), ShowtimeServer(){
	this->init(TCHAR_TO_UTF8(*name));
}
