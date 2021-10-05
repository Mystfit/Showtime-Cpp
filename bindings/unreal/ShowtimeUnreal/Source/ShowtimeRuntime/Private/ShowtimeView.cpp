// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMinimal.h"
#include "Actor.h"
#include "ShowtimeView.h"
#include <Runtime/Engine/Classes/Engine/World.h>

void UShowtimeView::RegisterSpawnedWrapper(AShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->init(this, entity_path);
	EntityWrappers.Add(entity_path, wrapper);
}

AShowtimeEntity* UShowtimeView::GetWrapperParent(const AShowtimeEntity* wrapper) const
{
	auto path = ZstURI(TCHAR_TO_UTF8(*wrapper->EntityPath));
	auto parent_path = path.parent();
	if (parent_path.is_empty())
		return nullptr;

	if (ZstEntityBase* entity = Handle()->find_entity(parent_path)) {
		if (auto val = EntityWrappers.Find(parent_path.path())) {
			return *val;
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

AShowtimeEntity* UShowtimeView::SpawnEntity(ZstEntityBase* entity)
{
	switch (entity->entity_type()) {
	case ZstEntityType::PERFORMER:
		return SpawnPerformer(static_cast<ZstPerformer*>(entity));
	case ZstEntityType::COMPONENT:
		return SpawnComponent(static_cast<ZstComponent*>(entity));
	case ZstEntityType::PLUG:
		return SpawnPlug(static_cast<ZstPlug*>(entity));
	case ZstEntityType::FACTORY:
		return SpawnFactory(static_cast<ZstEntityFactory*>(entity));
	default:
		break;
	}

	return nullptr;
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
		//Add component to parent component
		if (auto parent = entity_actor->GetParent()) {
			auto e_type = parent->GetNativeEntity()->entity_type();
			if (e_type == ZstEntityType::COMPONENT || e_type == ZstEntityType::PERFORMER) {
				auto parent_c = static_cast<AShowtimeComponent*>(parent);
				parent_c->ComponentAttached(entity_actor);
			}
		}

		RegisterSpawnedWrapper(entity_actor, component);
		return entity_actor;
	}
	return nullptr;
}

AShowtimeCable* UShowtimeView::SpawnCable(ZstCable* cable)
{
	if (auto cable_actor = GetWorld()->SpawnActor<AShowtimeCable>(SpawnableCable)) {
		CableWrappers.Add(cable->get_address(), cable_actor);
		cable_actor->OwningClient = this;
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
		if (auto parent = factory_actor->GetParent()) {
			auto e_type = parent->GetNativeEntity()->entity_type();
			if (e_type == ZstEntityType::PERFORMER) {
				auto parent_c = static_cast<AShowtimePerformer*>(parent);
				parent_c->FactoryAttached(factory_actor);
			}
		}

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

	if (auto plug_actor = GetWorld()->SpawnActor<AShowtimePlug>(SpawnablePlug))
	{
		RegisterSpawnedWrapper(plug_actor, plug);

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

void UShowtimeView::on_performer_arriving(ZstPerformer* performer)
{
	OnPerformerArriving.Broadcast(SpawnPerformer(performer));
}

void UShowtimeView::on_performer_leaving(const ZstURI& performer_path)
{
	//Owner->OnPerformerLeaving.Broadcast(performer_path);
}

void UShowtimeView::on_entity_arriving(ZstEntityBase* entity)
{
	OnEntityArriving.Broadcast(SpawnEntity(entity));
}

void UShowtimeView::on_entity_leaving(const ZstURI& entity_path)
{
	OnEntityUpdated.Broadcast(*Owner->EntityWrappers.Find(UTF8_TO_TCHAR(entity_path.path())));
}

void UShowtimeView::on_entity_updated(ZstEntityBase* entity)
{
	OnEntityUpdated.Broadcast(*Owner->EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path())));
}

void UShowtimeView::on_factory_arriving(ZstEntityFactory* factory)
{
	OnEntityArriving.Broadcast(SpawnFactory(factory));
}

void UShowtimeView::on_factory_leaving(const ZstURI& factory_path)
{
	OnEntityUpdated.Broadcast(*Owner->EntityWrappers.Find(UTF8_TO_TCHAR(factory_path.path())));
}

void UShowtimeView::on_cable_created(ZstCable* cable)
{
	OnCableCreated.Broadcast(SpawnCable(cable));
}

void UShowtimeView::on_cable_destroyed(const ZstCableAddress& cable_address)
{
	FShowtimeCableAddress address(cable_address);//{ UTF8_TO_TCHAR(cable_address.get_input_URI().path()), UTF8_TO_TCHAR(cable_address.get_output_URI().path())};
	auto cable_wrapper = Owner->CableWrappers.Find(address);
	if (cable_wrapper)
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
