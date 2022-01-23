// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeView.h"
#include "CoreMinimal.h"
#include "Async/Async.h"
#include "GameFramework/Actor.h"
#include "ShowtimeServerBeacon.h"
#include <Runtime/Engine/Classes/Engine/World.h>
#include <ShowtimeRuntime/Public/ShowtimeCableInterface.h>

UShowtimeView::UShowtimeView() 
{

}

void UShowtimeView::RegisterSpawnedWrapper(UShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	//FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->Init(entity->URI());
	EntityWrappers.Add(wrapper->URI->Path(), wrapper);
}

UShowtimeEntity* UShowtimeView::GetWrapper(const ZstEntityBase* entity) const
{
	if (!entity)
		return nullptr;
	return GetWrapper(entity->URI());
}

UShowtimeEntity* UShowtimeView::GetWrapper(const ZstURI& URI) const
{
	if (auto entity_wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(URI.path()))) {
		return *entity_wrapper;
	}
	return nullptr;
}

UShowtimeEntity* UShowtimeView::SpawnEntity(ZstEntityBase* entity)
{
	UShowtimeEntity* entity_comp = nullptr;

	switch (entity->entity_type()) {
	case ZstEntityType::PERFORMER:
		entity_comp = SpawnEntityActorFromPrototype<UShowtimePerformer>(entity, SpawnablePerformer);
		break;
	case ZstEntityType::COMPONENT:
		entity_comp = SpawnEntityActorFromPrototype<UShowtimeComponent>(entity, SpawnableComponent);
		break;
	case ZstEntityType::PLUG:
		entity_comp = SpawnEntityActorFromPrototype<UShowtimePlug>(entity, SpawnablePlug);
		break;
	case ZstEntityType::FACTORY:
		entity_comp = SpawnEntityActorFromPrototype<UShowtimeFactory>(entity, SpawnableFactory);
		break;
	default:
		break;
	}

	return entity_comp;
}

UShowtimeCable* UShowtimeView::SpawnCable(ZstCable* cable)
{
	if (auto cable_actor = GetWorld()->SpawnActor<AActor>(SpawnableCable)) {
		UShowtimeCable* cable_comp = cable_actor->FindComponentByClass<UShowtimeCable>();
		if (!cable_comp) {
			cable_comp = NewObject<UShowtimeCable>(cable_actor);
			cable_comp->RegisterComponent();
		}
		cable_comp->Address = FShowtimeCableAddress(cable->get_address());
		CableWrappers.Add(cable->get_address(), cable_comp);
		return cable_comp;
	}
	return nullptr;
}

UShowtimeServerBeacon* UShowtimeView::SpawnServerBeacon(const FServerAddress& address)
{
	auto world = GetWorld();
	if (!world)
		return nullptr;

	if (auto server_beacon_actor = world->SpawnActor<AActor>(SpawnableServer)) { 
		UShowtimeServerBeacon* beacon_comp = server_beacon_actor->FindComponentByClass<UShowtimeServerBeacon>();
		if (!beacon_comp) {
			beacon_comp = NewObject<UShowtimeServerBeacon>(server_beacon_actor);
			beacon_comp->RegisterComponent();
		}
		beacon_comp->Server = address;
		ServerBeaconWrappers.Add(beacon_comp->Server, beacon_comp);
		return beacon_comp;
	}
	return nullptr;
}

void UShowtimeView::PlaceEntity_Implementation(UShowtimeEntity* entity)
{
	if (!entity)
		return;

	if (!entity->GetNativeEntity())
		return;

	auto parent = entity->GetParent();
	if (!parent)
		return;

	//Add plug to parent component
	if (auto native_parent = entity->GetNativeEntity()) {
		switch (native_parent->entity_type()) {
		case ZstEntityType::COMPONENT:
			static_cast<UShowtimeComponent*>(parent)->ComponentPlaced(static_cast<UShowtimeComponent*>(entity));
			break;
		case ZstEntityType::PERFORMER:
			break;
		case ZstEntityType::PLUG:
			static_cast<UShowtimeComponent*>(parent)->PlugPlaced(static_cast<UShowtimePlug*>(entity));
			break;
		case ZstEntityType::FACTORY:
			if (native_parent->entity_type() == ZstEntityType::PERFORMER) {
				static_cast<UShowtimePerformer*>(parent)->FactoryPlaced(static_cast<UShowtimeFactory*>(entity));
			}
			break;
		}
	}
		

		// For reference only. Trigger placement events on all entities and parents affected
		//if (parent->GetNativeEntity()->entity_type() == ZstEntityType::COMPONENT) {
		//	auto parent_c = static_cast<UShowtimeComponent*>(parent);
		//	parent_c->PlugPlaced(plug_actor);
		//}
	
		//if (parent->GetNativeEntity()->entity_type() == ZstEntityType::PERFORMER) {
		//	static_cast<UShowtimePerformer*>(parent)->FactoryAtached(factory_actor);
		//}

		//auto e_type = parent->GetNativeEntity()->entity_type();
		//if (e_type == ZstEntityType::COMPONENT || e_type == ZstEntityType::PERFORMER) {
		//	static_cast<UShowtimeComponent*>(parent)->ComponentPlaced(entity_actor);
		//}
	
}

void UShowtimeView::PlaceCable_Implementation(UShowtimeCable* cable)
{
	UShowtimePlug* input_plug_entity = cable->GetInputPlug();
	UShowtimeEntity* output_plug_entity = cable->GetOutputPlug();
	if (input_plug_entity && output_plug_entity) {
		// Cable already has plugs defined
	}
}

void UShowtimeView::on_performer_arriving(ZstPerformer* performer)
{
	AsyncTask(ENamedThreads::GameThread, [this, performer]() {
		if (auto performer_actor = SpawnEntityActorFromPrototype<UShowtimePerformer>(performer, SpawnablePerformer)) {
			PlaceEntity(performer_actor);
			OnPerformerArriving.Broadcast(performer_actor);
		}
	});
}

void UShowtimeView::on_performer_leaving(const ZstURI& performer_path)
{
	if (auto entity_wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(performer_path.path()))) {
		if(auto performer_wrapper = Cast<UShowtimePerformer>(*entity_wrapper))
			OnPerformerLeaving.Broadcast(performer_wrapper);
	}
}

void UShowtimeView::on_entity_arriving(ZstEntityBase* entity)
{
	AsyncTask(ENamedThreads::GameThread, [this, entity]() {
		if (auto entity_actor = SpawnEntity(entity)) {
			PlaceEntity(entity_actor);
			OnEntityArriving.Broadcast(entity_actor);
		}
	});
}

void UShowtimeView::on_entity_leaving(const ZstURI& entity_path)
{
	if (auto entity_wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(entity_path.path()))) {
		OnEntityLeaving.Broadcast(*entity_wrapper);
	}
}

void UShowtimeView::on_entity_updated(ZstEntityBase* entity)
{
	if (!entity)
		return;

	if (auto entity_wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path()))) {
		OnEntityUpdated.Broadcast(*entity_wrapper);
	}
}

void UShowtimeView::on_factory_arriving(ZstEntityFactory* factory)
{
	AsyncTask(ENamedThreads::GameThread, [this, factory]() {
		OnEntityArriving.Broadcast(SpawnEntityActorFromPrototype<UShowtimeFactory>(factory, SpawnableFactory));
	});
}

void UShowtimeView::on_factory_leaving(const ZstURI& factory_path)
{
	if (auto entity_wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(factory_path.path()))) {
		OnEntityUpdated.Broadcast(*entity_wrapper);
	}
}

void UShowtimeView::on_cable_created(ZstCable* cable)
{
	AsyncTask(ENamedThreads::GameThread, [this, cable]() {
		UShowtimeCable* cable_wrapper = nullptr;
		UShowtimeCable** cable_wrapper_it = CableWrappers.Find(FShowtimeCableAddress(cable->get_address()));
		if (cable_wrapper_it) {
			// Cable exists already
			cable_wrapper = *cable_wrapper_it;
		}
		else {
			cable_wrapper = SpawnCable(cable);
			if(cable_wrapper->GetOwner()->Implements<UShowtimeCableInterface>()){
				IShowtimeCableInterface::Execute_ConnectCableToEndpoints(cable_wrapper->GetOwner(), cable_wrapper->GetInputPlug(), cable_wrapper->GetOutputPlug());
			}
		}
	
		PlaceCable(cable_wrapper);
		OnCableCreated.Broadcast(cable_wrapper);
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
