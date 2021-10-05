#include "ClientAdaptors.h"
#include "ShowtimeClient.h"
//#ifdef PLATFORM_ANDROID
//#include "MulticastAndroid.h"
//#endif

using namespace showtime;

ClientAdaptors::ClientAdaptors(UShowtimeClient* owner) : Owner(owner)
{
}

void ClientAdaptors::on_connected_to_server(ShowtimeClient* client, const ZstServerAddress* server)
{
	//Owner->RefreshEntityWrappers();
	Owner->OnConnectedToServer.Broadcast(Owner, FServerAddressFromShowtime(server));
#if PLATFORM_ANDROID
	//MulticastAndroid::ReleaseMulticastLock();
#endif
}

void ClientAdaptors::on_disconnected_from_server(ShowtimeClient* client, const ZstServerAddress* server)
{
	Owner->OnDisconnectedFromServer.Broadcast(Owner, FServerAddressFromShowtime(server));
#if PLATFORM_ANDROID
	//MulticastAndroid::AcquireMulticastLock();
#endif
}

void ClientAdaptors::on_server_discovered(ShowtimeClient* client, const ZstServerAddress* server)
{
	UE_LOG(Showtime, Display, TEXT("Received server beacon %s"), UTF8_TO_TCHAR(server->c_name()));
	Owner->OnServerDiscovered.Broadcast(Owner, Owner->SpawnServerBeacon(server));
}

void ClientAdaptors::on_server_lost(ShowtimeClient* client, const ZstServerAddress* server)
{
	Owner->OnServerLost.Broadcast(Owner, *Owner->ServerBeaconWrappers.Find(FServerAddressFromShowtime(server)));
}

void ClientAdaptors::on_synchronised_graph(ShowtimeClient* client, const ZstServerAddress* server)
{
	Owner->OnGraphSynchronised.Broadcast(Owner, FServerAddressFromShowtime(server));
}

void ClientAdaptors::on_formatted_log_record(const char* record)
{
	FString message(UTF8_TO_TCHAR(record));
	UE_LOG(Showtime, Display, TEXT("%s"), *message);
}
