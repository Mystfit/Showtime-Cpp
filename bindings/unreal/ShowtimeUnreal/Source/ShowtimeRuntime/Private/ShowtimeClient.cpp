// Fill out your copyright notice in the Description page of Project Settings.
#include "ShowtimeClient.h"
#include "ShowtimePerformer.h"
#include <showtime/ShowtimeClient.h>

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#ifdef PLATFORM_ANDROID
#include "MulticastAndroid.h"
#include "ShowtimePluginManagerAndroid.h"
#endif
#include <functional>

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient() : 
	client(MakeShared<ShowtimeClient>())
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShowtimeClient::Cleanup()
{
	RemoveEvents();
#if PLATFORM_ANDROID
	UMulticastAndroid::ReleaseMulticastLock();
#endif
	if (client) client->destroy();
}

void UShowtimeClient::Init()
{
	FString plugin_path;

#if PLATFORM_ANDROID
	UMulticastAndroid::AcquireMulticastLock();
	plugin_path = UShowtimePluginManagerAndroid::GetPluginPath();
#endif
	if (client) {
		client->set_plugin_path(TCHAR_TO_UTF8(*plugin_path));
		client->init(TCHAR_TO_UTF8(*ClientName), true);
	}
	SpawnPerformer(Handle()->get_root());
}

void UShowtimeClient::JoinServerByName(const FString& name)
{
	if (client) client->auto_join_by_name_async(TCHAR_TO_UTF8(*name));
}

void UShowtimeClient::LeaveServer()
{
	if (client) client->leave();
}

bool UShowtimeClient::IsConnected() const
{
	return (client) ? client->is_connected() : false;
}

TArray<UShowtimePerformer*> UShowtimeClient::GetPerformers() const
{
	TArray<UShowtimePerformer*> performer_wrappers;

	auto performers = std::make_shared<ZstEntityBundle>();
	Handle()->get_performers(performers.get());

	for (int i = 0; i < performers->size(); ++i) {
		auto entity = performers->item_at(i);
		if (!entity)
			continue;

		if (entity->entity_type() != ZstEntityType::PERFORMER)
			continue;

		auto performer_wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path()));
		if (performer_wrapper)
			performer_wrappers.Add(static_cast<UShowtimePerformer*>(*performer_wrapper));
	}

	return performer_wrappers;
}

UShowtimePerformer* UShowtimeClient::GetRootPerformer() const
{
	auto wrapper = EntityWrappers.Find(UTF8_TO_TCHAR(Handle()->get_root()->URI().path()));
	if (wrapper) {
		return static_cast<UShowtimePerformer*>(*wrapper);
	}
	return nullptr;
}

void UShowtimeClient::ConnectCable(UShowtimePlug* InputPlug, UShowtimePlug* OutputPlug) const
{
	if (!InputPlug || !OutputPlug) {
		UE_LOG(Showtime, Display, TEXT("Input or Outplug plug was null"));
		return;
	}

	Handle()->connect_cable(static_cast<ZstInputPlug*>(InputPlug->GetNativePlug()), static_cast<ZstOutputPlug*>(OutputPlug->GetNativePlug()));
}

void UShowtimeClient::BeginPlay()
{
	AttachEvents();
	Super::BeginPlay();
}

void UShowtimeClient::BeginDestroy()
{
	Super::BeginDestroy();
	Cleanup();
}

void UShowtimeClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Cleanup();
}

TSharedPtr<ShowtimeClient> UShowtimeClient::Handle() const
{
	return client;
}

void UShowtimeClient::RefreshEntityWrappers()
{
	auto entities = std::make_shared<ZstEntityBundle>();
	auto performers = std::make_shared<ZstEntityBundle>();

	// Get flat list of entities from performers
	Handle()->get_performers(performers.get());
	size_t i;
	size_t num_performers = performers->size();
	for (i = 0; i < num_performers; ++i) {
		auto performer = performers->item_at(i);
		if(performer)
			performer->get_child_entities(entities.get(), true, true);
	}

	// Refresh wrappers from all entities
	size_t num_entities = entities->size();
	for (i = 0; i < num_entities; ++i) {
		auto entity = entities->item_at(i);
		if(entity)
			SpawnEntity(entity);
	}
}

UShowtimeEntity* UShowtimeClient::SpawnEntity(ZstEntityBase* entity)
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

UShowtimePerformer* UShowtimeClient::SpawnPerformer(ZstPerformer* performer)
{
	
	auto entity_actor = GetWorld()->SpawnActor<AActor>(SpawnablePerformer);
	if (!entity_actor)
		return nullptr;
	
	auto entity_wrapper = entity_actor->FindComponentByClass<UShowtimePerformer>();

	if (!entity_wrapper)
		entity_wrapper = NewObject<UShowtimePerformer>(entity_actor);

	RegisterSpawnedWrapper(entity_wrapper, performer);
	return entity_wrapper;
}

UShowtimeComponent* UShowtimeClient::SpawnComponent(ZstComponent* component) 
{
	auto entity_actor = GetWorld()->SpawnActor<AActor>(SpawnableComponent);
	if (!entity_actor)
		return nullptr;

	auto entity_wrapper = entity_actor->FindComponentByClass<UShowtimeComponent>();

	if (!entity_wrapper)
		entity_wrapper = NewObject<UShowtimeComponent>(entity_actor);

	//Add component to parent component
	if (auto parent = entity_wrapper->GetParent()) {
		auto e_type = parent->GetNativeEntity()->entity_type();
		if (e_type == ZstEntityType::COMPONENT || e_type == ZstEntityType::PERFORMER) {
			auto parent_c = static_cast<UShowtimeComponent*>(parent);
			parent_c->ComponentAttached(entity_wrapper);
		}
	}

	RegisterSpawnedWrapper(entity_wrapper, component);
	return entity_wrapper;
}

AShowtimeCable* UShowtimeClient::SpawnCable(ZstCable* cable)
{
	return nullptr;
}

UShowtimeFactory* UShowtimeClient::SpawnFactory(ZstEntityFactory* factory)
{
	if (auto factory_actor = GetWorld()->SpawnActor<AActor>(SpawnableFactory)){
		auto entity_wrapper = factory_actor->FindComponentByClass<UShowtimeFactory>();

		if (!entity_wrapper)
			entity_wrapper = NewObject<UShowtimeFactory>(factory_actor);

		if (auto parent = entity_wrapper->GetParent()) {
			auto e_type = parent->GetNativeEntity()->entity_type();
			if (e_type == ZstEntityType::PERFORMER) {
				auto parent_c = static_cast<UShowtimePerformer*>(parent);
				parent_c->FactoryAttached(entity_wrapper);
			}
		}

		RegisterSpawnedWrapper(entity_wrapper, factory);
	}
	return nullptr;
}

UShowtimePlug* UShowtimeClient::SpawnPlug(ZstPlug* plug) 
{
	//auto transform = FTransform();
	//auto plug_actor =  Cast<AActor>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, SpawnablePlug->GetClass(), transform));
	if (auto plug_actor = GetWorld()->SpawnActor<AActor>(SpawnablePlug))
	{
		auto entity_wrapper = plug_actor->FindComponentByClass<UShowtimePlug>();
		if (!entity_wrapper) {
			entity_wrapper = NewObject<UShowtimePlug>(plug_actor);
			entity_wrapper->RegisterComponent();
		}
		RegisterSpawnedWrapper(entity_wrapper, plug);

		// Finish spawning the actor
		//UGameplayStatics::FinishSpawningActor(plug_actor, transform);

		//Add plug to parent component
		if (auto parent = entity_wrapper->GetParent()) {
			if (parent->GetNativeEntity()->entity_type() == ZstEntityType::COMPONENT) {
				auto parent_c = static_cast<UShowtimeComponent*>(parent);
				parent_c->PlugAttached(entity_wrapper);
			}
		}
		return entity_wrapper;

	}

	return nullptr;
}

void UShowtimeClient::RegisterSpawnedWrapper(UShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->init(this, entity_path);
	EntityWrappers.Add(entity_path, wrapper);
}

UShowtimeEntity* UShowtimeClient::GetWrapperParent(const UShowtimeEntity* wrapper) const
{
	auto path = ZstURI(TCHAR_TO_UTF8(*wrapper->EntityPath));
	auto parent_path = path.parent();
	if (parent_path.is_empty())
		return nullptr;

	if (ZstEntityBase* entity = Handle()->find_entity(parent_path)) {
		if (auto val = EntityWrappers.Find(UTF8_TO_TCHAR(parent_path.path()))) {
			return *val;
		}
	}
	return nullptr;
}


void UShowtimeClient::AttachEvents(){
	if (!client)
		return;

	client_adaptor = std::make_shared<ClientAdaptors>(this);
	client->add_connection_adaptor(client_adaptor);
	client->add_hierarchy_adaptor(client_adaptor);
	client->add_log_adaptor(client_adaptor);
}

void UShowtimeClient::RemoveEvents(){
	if (!client)
		return;

	client->remove_connection_adaptor(client_adaptor);
	client->remove_hierarchy_adaptor(client_adaptor);
	client->remove_log_adaptor(client_adaptor);
}

void UShowtimeClient::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if(client) client->poll_once();
}
