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
	Owner->OnPerformerArriving.Broadcast(Owner->GetWorld()->SpawnActor<AShowtimePerformer>(Owner->SpawnablePerformer->StaticClass()));
}

void ClientAdaptors::on_performer_leaving(const ZstURI& performer_path)
{
	//Owner->OnPerformerLeaving.Broadcast(performer_path);
}

void ClientAdaptors::on_entity_arriving(ZstEntityBase* entity)
{
	switch (entity->entity_type()) {
	case ZstEntityType::PLUG:
		Owner->OnEntityArriving.Broadcast(Owner->GetWorld()->SpawnActor<AShowtimePlug>(Owner->SpawnablePlug->StaticClass()));
		break;
	case ZstEntityType::COMPONENT:
		Owner->OnEntityArriving.Broadcast(Owner->GetWorld()->SpawnActor<AShowtimeComponent>(Owner->SpawnableComponent->StaticClass()));
		break;
	default:
		break;
	}
	//Owner->OnEntityArriving.Broadcast(Owner->GetWorld()->SpawnActor<AShowtimeEntity>(Owner->SpawnableEntity->StaticClass()));
	//Owner->OnEntityArriving.Broadcast(entity);
}

void ClientAdaptors::on_entity_leaving(const ZstURI& entity_path)
{
	//Owner->OnEntityLeaving.Broadcast(entity_path);
}

void ClientAdaptors::on_entity_updated(ZstEntityBase* entity)
{
	//Owner->OnEntityUpdated.Broadcast(entity);
}

void ClientAdaptors::on_factory_arriving(ZstEntityFactory* factory)
{
	Owner->OnEntityArriving.Broadcast(Owner->GetWorld()->SpawnActor<AShowtimeFactory>(Owner->SpawnableFactory->StaticClass()));
	//Owner->OnFactoryArriving.Broadcast(factory);
}

void ClientAdaptors::on_factory_leaving(const ZstURI& factory_path)
{
	//Owner->OnFactoryLeaving.Broadcast(factory_path);
}

void ClientAdaptors::on_cable_created(ZstCable* cable)
{
	//Owner->OnCableArriving.Broadcast(cable);
}

void ClientAdaptors::on_cable_destroyed(const ZstCableAddress& cable_address)
{
	//Owner->OnCableDestroyed.Broadcast(cable_address);
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
