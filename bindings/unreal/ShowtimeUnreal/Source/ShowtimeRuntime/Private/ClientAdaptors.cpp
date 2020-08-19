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
	Owner->OnServerDiscovered.Broadcast(Owner, FServerAddressFromShowtime(server));
}

void ClientAdaptors::on_server_lost(ShowtimeClient* client, const ZstServerAddress* server)
{
	Owner->OnServerLost.Broadcast(Owner, FServerAddressFromShowtime(server));
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

void ClientAdaptors::on_performer_arriving(ZstPerformer* performer)
{
	Owner->OnPerformerArriving.Broadcast(Owner->SpawnPerformer(performer));
}

void ClientAdaptors::on_performer_leaving(const ZstURI& performer_path)
{
	//Owner->OnPerformerLeaving.Broadcast(performer_path);
}

void ClientAdaptors::on_entity_arriving(ZstEntityBase* entity)
{
	Owner->OnEntityArriving.Broadcast(Owner->SpawnEntity(entity));
}

void ClientAdaptors::on_entity_leaving(const ZstURI& entity_path)
{
	Owner->OnEntityUpdated.Broadcast(*Owner->EntityWrappers.Find(UTF8_TO_TCHAR(entity_path.path())));
}

void ClientAdaptors::on_entity_updated(ZstEntityBase* entity)
{
	Owner->OnEntityUpdated.Broadcast(*Owner->EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path())));
}

void ClientAdaptors::on_factory_arriving(ZstEntityFactory* factory)
{
	Owner->OnEntityArriving.Broadcast(Owner->SpawnFactory(factory));
}

void ClientAdaptors::on_factory_leaving(const ZstURI& factory_path)
{
	Owner->OnEntityUpdated.Broadcast(*Owner->EntityWrappers.Find(UTF8_TO_TCHAR(factory_path.path())));
}

void ClientAdaptors::on_cable_created(ZstCable* cable)
{
	Owner->OnCableCreated.Broadcast(Owner->SpawnCable(cable));
}

void ClientAdaptors::on_cable_destroyed(const ZstCableAddress& cable_address)
{
	FShowtimeCableAddress address{ UTF8_TO_TCHAR(cable_address.get_input_URI().path()), UTF8_TO_TCHAR(cable_address.get_output_URI().path())};
	auto cable_wrapper = Owner->CableWrappers.Find(address);
	if(cable_wrapper)
		Owner->OnCableDestroyed.Broadcast(*cable_wrapper);
}

//void ClientAdaptors::on_plugin_loaded(std::shared_ptr<ZstPlugin> plugin)
//{
//	Owner->OnPluginLoaded.Broadcast(plugin);
//}
//
//void ClientAdaptors::on_plugin_unloaded(std::shared_ptr<ZstPlugin> plugin)
//{
//	Owner->OnPluginUnloaded.Broadcast(plugin);
//}
//
