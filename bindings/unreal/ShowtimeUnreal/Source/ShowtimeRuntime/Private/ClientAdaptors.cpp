#include "ClientAdaptors.h"
#include "ShowtimeSubsystem.h"
#if PLATFORM_ANDROID
#include "MulticastAndroid.h"
#endif

using namespace showtime;

ClientAdaptors::ClientAdaptors(UShowtimeSubsystem* owner) : Owner(owner)
{
}

void ClientAdaptors::on_connected_to_server(ShowtimeClient* client, const ZstServerAddress* server)
{
	//Owner->RefreshEntityWrappers();
	Owner->OnConnectedToServer.Broadcast(FServerAddressFromShowtime(server));
#if PLATFORM_ANDROID
	// If we are connected, we don't need to receive server beacons
	UMulticastAndroid::ReleaseMulticastLock();
#endif
}

void ClientAdaptors::on_disconnected_from_server(ShowtimeClient* client, const ZstServerAddress* server)
{
	Owner->OnDisconnectedFromServer.Broadcast(FServerAddressFromShowtime(server));
#if PLATFORM_ANDROID
	// Start listening for server beacons again
	UMulticastAndroid::AcquireMulticastLock();
#endif
}

void ClientAdaptors::on_synchronised_graph(ShowtimeClient* client, const ZstServerAddress* server)
{
	Owner->OnGraphSynchronised.Broadcast(FServerAddressFromShowtime(server));
}

void ClientAdaptors::on_formatted_log_record(const char* record)
{
	FString message(UTF8_TO_TCHAR(record));
	UE_LOG(Showtime, Display, TEXT("%s"), *message);
}