// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeView.h"
#include "CoreMinimal.h"
#include "Async/Async.h"
#include "GameFramework/Actor.h"
#include "ShowtimeServerBeacon.h"
#include <Runtime/Engine/Classes/Engine/World.h>

UShowtimeView::UShowtimeView() 
{

}

void UShowtimeView::RegisterSpawnedWrapper(AShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->init(entity_path);
	EntityWrappers.Add(entity_path, wrapper);
}

//AShowtimeEntity* UShowtimeView::GetWrapperParent(const AShowtimeEntity* wrapper) const
//{
//	auto path = ZstURI(TCHAR_TO_UTF8(*wrapper->EntityPath));
//	auto parent_path = path.parent();
//	if (parent_path.is_empty())
//		return nullptr;
//
//
//	if (auto client = GetOwner()) {
//		if (ZstEntityBase* entity = client->Handle()->find_entity(parent_path)) {
//			if (auto val = EntityWrappers.Find(parent_path.path())) {
//				return *val;
//			}
//		}
//	}
//
//	return nullptr;
//}

AShowtimeEntity* UShowtimeView::GetWrapper(const ZstEntityBase* entity) const
{
	return *EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path()));
}

AShowtimeEntity* UShowtimeView::GetWrapper(const ZstURI& URI) const
{
	return *EntityWrappers.Find(UTF8_TO_TCHAR(URI.path()));
}

AShowtimeEntity* UShowtimeView::SpawnEntity(ZstEntityBase* entity)
{
	AShowtimeEntity* entity_actor = nullptr;

	switch (entity->entity_type()) {
	case ZstEntityType::PERFORMER:
		entity_actor = SpawnPerformer(static_cast<ZstPerformer*>(entity));
	case ZstEntityType::COMPONENT:
		entity_actor = SpawnComponent(static_cast<ZstComponent*>(entity));
	case ZstEntityType::PLUG:
		entity_actor = SpawnPlug(static_cast<ZstPlug*>(entity));
	case ZstEntityType::FACTORY:
		entity_actor = SpawnFactory(static_cast<ZstEntityFactory*>(entity));
	default:
		break;
	}

	return entity_actor;
}

AShowtimePerformer* UShowtimeView::SpawnPerformer(ZstPerformer* performer)
{
	// Create a new performer actor from our template performer
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(performer->URI().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto entity_actor = GetWorld()->SpawnActor<AShowtimePerformer>(SpawnablePerformer, params)) {
		RegisterSpawnedWrapper(entity_actor, performer);
		return entity_actor;
	}
	return nullptr;
}

AShowtimeComponent* UShowtimeView::SpawnComponent(ZstComponent* component)
{
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(component->URI().last().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto entity_actor = GetWorld()->SpawnActor<AShowtimeComponent>(SpawnableComponent))
	{
		RegisterSpawnedWrapper(entity_actor, component);
		return entity_actor;
	}
	return nullptr;
}

AShowtimeCable* UShowtimeView::SpawnCable(ZstCable* cable)
{
	if (auto cable_actor = GetWorld()->SpawnActor<AShowtimeCable>(SpawnableCable)) {
		CableWrappers.Add(cable->get_address(), cable_actor);
		return cable_actor;
	}
	return nullptr;
}


AShowtimeFactory* UShowtimeView::SpawnFactory(ZstEntityFactory* factory)
{
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(factory->URI().last().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

	if (auto factory_actor = GetWorld()->SpawnActor<AShowtimeFactory>(SpawnableFactory)) {
		RegisterSpawnedWrapper(factory_actor, factory);
	}
	return nullptr;
}

AShowtimePlug* UShowtimeView::SpawnPlug(ZstPlug* plug)
{
	//auto transform = FTransform();
	//auto plug_actor =  Cast<AActor>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, SpawnablePlug->GetClass(), transform));
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(plug->URI().last().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
	if (auto plug_actor = GetWorld()->SpawnActor<AShowtimePlug>(SpawnablePlug, params))
	{
		RegisterSpawnedWrapper(plug_actor, plug);
		return plug_actor;
	}

	return nullptr;
}


AShowtimeServerBeacon* UShowtimeView::SpawnServerBeacon(const FServerAddress& address)
{
	if (auto server_beacon_actor = GetWorld()->SpawnActor<AShowtimeServerBeacon>(SpawnableServer)) {
		server_beacon_actor->Server = address;
		ServerBeaconWrappers.Add(server_beacon_actor->Server, server_beacon_actor);
		return server_beacon_actor;
	}
	return nullptr;
}

void UShowtimeView::PlaceEntity_Implementation(AShowtimeEntity* entity)
{
	if (!entity)
		return;

	if (!entity->GetNativeEntity())
		return;

	//Add plug to parent component
	if (auto parent = entity->GetParent()) {
		switch (entity->GetNativeEntity()->entity_type()) {
		case ZstEntityType::COMPONENT:
			break;
		case ZstEntityType::PERFORMER:
			break;
		case ZstEntityType::PLUG:
			break;
		case ZstEntityType::FACTORY:
			break;
		}

		// For reference only. Trigger placement events on all entities and parents affected
		//if (parent->GetNativeEntity()->entity_type() == ZstEntityType::COMPONENT) {
		//	auto parent_c = static_cast<AShowtimeComponent*>(parent);
		//	parent_c->PlugAttached(plug_actor);
		//}
	
		//if (parent->GetNativeEntity()->entity_type() == ZstEntityType::PERFORMER) {
		//	static_cast<AShowtimePerformer*>(parent)->FactoryAttached(factory_actor);
		//}

		//auto e_type = parent->GetNativeEntity()->entity_type();
		//if (e_type == ZstEntityType::COMPONENT || e_type == ZstEntityType::PERFORMER) {
		//	static_cast<AShowtimeComponent*>(parent)->ComponentAttached(entity_actor);
		//}
	}
}

void UShowtimeView::on_performer_arriving(ZstPerformer* performer)
{
	OnPerformerArriving.Broadcast(SpawnPerformer(performer));
}

void UShowtimeView::on_performer_leaving(const ZstURI& performer_path)
{
	if (auto entity_wrapper = *PerformerWrappers.Find(UTF8_TO_TCHAR(performer_path.path()))) {
		OnPerformerLeaving.Broadcast(entity_wrapper);
	}
}

void UShowtimeView::on_entity_arriving(ZstEntityBase* entity)
{
	AsyncTask(ENamedThreads::GameThread, [this, entity]() {
		auto entity_actor = SpawnEntity(entity);
		if (entity_actor) {
			PlaceEntity(entity_actor);
			OnEntityArriving.Broadcast(entity_actor);
		}
	});
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
	AsyncTask(ENamedThreads::GameThread, [this, factory]() {
		OnEntityArriving.Broadcast(SpawnFactory(factory));
	});
}

void UShowtimeView::on_factory_leaving(const ZstURI& factory_path)
{
	if (auto entity_wrapper = *EntityWrappers.Find(UTF8_TO_TCHAR(factory_path.path()))) {
		OnEntityUpdated.Broadcast(entity_wrapper);
	}
}

void UShowtimeView::on_cable_created(ZstCable* cable)
{
	AsyncTask(ENamedThreads::GameThread, [this, cable]() {
		OnCableCreated.Broadcast(SpawnCable(cable));
	});
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
	FServerAddress address = FServerAddressFromShowtime(server);
	AsyncTask(ENamedThreads::GameThread, [this, address]() {
		UE_LOG(Showtime, Display, TEXT("Received server beacon %s"), *address.name);
		OnServerDiscovered.Broadcast(SpawnServerBeacon(address));
	});
}

void UShowtimeView::on_server_lost(ShowtimeClient* client, const ZstServerAddress* server)
{
	OnServerLost.Broadcast(*ServerBeaconWrappers.Find(FServerAddressFromShowtime(server)));
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
