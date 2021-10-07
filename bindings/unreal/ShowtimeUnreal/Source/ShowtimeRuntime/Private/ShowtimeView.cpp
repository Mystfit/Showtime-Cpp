// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeView.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <Runtime/Engine/Classes/Engine/World.h>

void UShowtimeView::RegisterSpawnedWrapper(AShowtimeEntity* wrapper, const UShowtimeURI* path)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(*path->path());
	if (auto client = GetOwner()) {
		wrapper->init(client, entity_path);
	}
	EntityWrappers.Add(entity_path, wrapper);
}

AShowtimeEntity* UShowtimeView::GetWrapperParent(const AShowtimeEntity* wrapper) const
{
	auto path = ZstURI(TCHAR_TO_UTF8(*wrapper->EntityPath));
	auto parent_path = path.parent();
	if (parent_path.is_empty())
		return nullptr;


	if (auto client = GetOwner()) {
		if (ZstEntityBase* entity = client->Handle()->find_entity(parent_path)) {
			if (auto val = EntityWrappers.Find(parent_path.path())) {
				return *val;
			}
		}
	}

	return nullptr;
}

AShowtimeEntity* UShowtimeView::GetWrapper(const ZstEntityBase* entity) const
{
	return *EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path()));
}

AShowtimeEntity* UShowtimeView::GetWrapper(const ZstURI& URI) const
{
	return *EntityWrappers.Find(UTF8_TO_TCHAR(URI.path()));
}

UShowtimeClient* UShowtimeView::GetOwner() const
{
	auto outer = GetOuter();
	if (!outer)
		return nullptr;

	return static_cast<UShowtimeClient*>(outer);
}

AShowtimeEntity* UShowtimeView::SpawnEntity(ZstEntityBase* entity)
{
	UShowtimeURI path(entity->URI());
	switch (entity->entity_type()) {
	case ZstEntityType::PERFORMER:
		return SpawnPerformer(&path);
	case ZstEntityType::COMPONENT:
		return SpawnComponent(&path);
	case ZstEntityType::PLUG:
		return SpawnPlug(&path);
	case ZstEntityType::FACTORY:
		return SpawnFactory(&path);
	default:
		break;
	}

	return nullptr;
}

AShowtimePerformer* UShowtimeView::SpawnPerformer_Implementation(const UShowtimeURI* path)
{
	// Create a new performer actor from our template performer
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(*path->path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto entity_actor = GetWorld()->SpawnActor<AShowtimePerformer>(SpawnablePerformer, params)) {
		RegisterSpawnedWrapper(entity_actor, path);
		return entity_actor;
	}
	return nullptr;
}

AShowtimeComponent* UShowtimeView::SpawnComponent_Implementation(const UShowtimeURI* path)
{
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(*path->last().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto entity_actor = GetWorld()->SpawnActor<AShowtimeComponent>(SpawnableComponent))
	{
		//Add component to parent component
		if (auto parent = entity_actor->GetParent()) {
			auto e_type = parent->GetNativeEntity()->entity_type();
			if (e_type == ZstEntityType::COMPONENT || e_type == ZstEntityType::PERFORMER) {
				auto parent_c = static_cast<AShowtimeComponent*>(parent);
				parent_c->ComponentAttached(entity_actor);
			}
		}

		RegisterSpawnedWrapper(entity_actor, path);
		return entity_actor;
	}
	return nullptr;
}

AShowtimeCable* UShowtimeView::SpawnCable(ZstCable* cable)
{
	if (auto cable_actor = GetWorld()->SpawnActor<AShowtimeCable>(SpawnableCable)) {
		CableWrappers.Add(cable->get_address(), cable_actor);
		cable_actor->OwningClient = GetOwner();
		return cable_actor;
	}
	return nullptr;
}


AShowtimeFactory* UShowtimeView::SpawnFactory_Implementation(const UShowtimeURI* path)
{
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(path->last().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto factory_actor = GetWorld()->SpawnActor<AShowtimeFactory>(SpawnableFactory)) {
		if (auto parent = factory_actor->GetParent()) {
			auto e_type = parent->GetNativeEntity()->entity_type();
			if (e_type == ZstEntityType::PERFORMER) {
				auto parent_c = static_cast<AShowtimePerformer*>(parent);
				parent_c->FactoryAttached(factory_actor);
			}
		}

		RegisterSpawnedWrapper(factory_actor, path);
	}
	return nullptr;
}

AShowtimePlug* UShowtimeView::SpawnPlug_Implementation(const UShowtimeURI* path)
{
	//auto transform = FTransform();
	//auto plug_actor =  Cast<AActor>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, SpawnablePlug->GetClass(), transform));
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(*path->last().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto plug_actor = GetWorld()->SpawnActor<AShowtimePlug>(SpawnablePlug))
	{
		RegisterSpawnedWrapper(plug_actor, path);

		// Finish spawning the actor
		//UGameplayStatics::FinishSpawningActor(plug_actor, transform);

		//Add plug to parent component
		if (auto parent = plug_actor->GetParent()) {
			if (parent->GetNativeEntity()->entity_type() == ZstEntityType::COMPONENT) {
				auto parent_c = static_cast<AShowtimeComponent*>(parent);
				parent_c->PlugAttached(plug_actor);
			}
		}
		return plug_actor;
	}

	return nullptr;
}


AShowtimeServerBeacon* UShowtimeView::SpawnServerBeacon(const ZstServerAddress* server)
{
	if (auto server_beacon_actor = GetWorld()->SpawnActor<AShowtimeServerBeacon>(SpawnableServer)) {
		server_beacon_actor->OwningClient = GetOwner();
		server_beacon_actor->Server = FServerAddressFromShowtime(server);
		ServerBeaconWrappers.Add(server_beacon_actor->Server, server_beacon_actor);
		return server_beacon_actor;
	}
	return nullptr;
}

void UShowtimeView::on_performer_arriving(ZstPerformer* performer)
{
	UShowtimeURI path(performer->URI());
	OnPerformerArriving.Broadcast(SpawnPerformer(&path));
}

void UShowtimeView::on_performer_leaving(const ZstURI& performer_path)
{
	if (auto entity_wrapper = *PerformerWrappers.Find(UTF8_TO_TCHAR(performer_path.path()))) {
		OnPerformerLeaving.Broadcast(entity_wrapper);
	}
}

void UShowtimeView::on_entity_arriving(ZstEntityBase* entity)
{
	OnEntityArriving.Broadcast(SpawnEntity(entity));
}

void UShowtimeView::on_entity_leaving(const ZstURI& entity_path)
{
	if (auto entity_wrapper = *EntityWrappers.Find(UTF8_TO_TCHAR(entity_path.path()))) {
		OnEntityLeaving.Broadcast(*EntityWrappers.Find(UTF8_TO_TCHAR(entity_path.path())));
	}
}

void UShowtimeView::on_entity_updated(ZstEntityBase* entity)
{
	OnEntityUpdated.Broadcast(*EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path())));
}

void UShowtimeView::on_factory_arriving(ZstEntityFactory* factory)
{
	UShowtimeURI path(factory->URI());
	OnEntityArriving.Broadcast(SpawnFactory(&path));
}

void UShowtimeView::on_factory_leaving(const ZstURI& factory_path)
{
	if (auto entity_wrapper = *EntityWrappers.Find(UTF8_TO_TCHAR(factory_path.path()))) {
		OnEntityUpdated.Broadcast(entity_wrapper);
	}
}

void UShowtimeView::on_cable_created(ZstCable* cable)
{
	OnCableCreated.Broadcast(SpawnCable(cable));
}

void UShowtimeView::on_cable_destroyed(const ZstCableAddress& cable_address)
{
	FShowtimeCableAddress address(cable_address);//{ UTF8_TO_TCHAR(cable_address.get_input_URI().path()), UTF8_TO_TCHAR(cable_address.get_output_URI().path())};
	auto cable_wrapper = CableWrappers.Find(address);
	if (cable_wrapper)
		OnCableDestroyed.Broadcast(*cable_wrapper);
}

void UShowtimeView::on_server_discovered(ShowtimeClient* client, const ZstServerAddress* server)
{
	UE_LOG(Showtime, Display, TEXT("Received server beacon %s"), UTF8_TO_TCHAR(server->c_name()));
	OnServerDiscovered.Broadcast(GetOwner(), SpawnServerBeacon(server));
}

void UShowtimeView::on_server_lost(ShowtimeClient* client, const ZstServerAddress* server)
{
	OnServerLost.Broadcast(GetOwner(), *ServerBeaconWrappers.Find(FServerAddressFromShowtime(server)));
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
