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

TArray<AShowtimePerformer*> UShowtimeClient::GetPerformers() const
{
	TArray<AShowtimePerformer*> performer_wrappers = TArray<AShowtimePerformer*>();

	auto performers = std::make_shared<ZstEntityBundle>();
	Handle()->get_performers(performers.get());

	for (int i = 0; i < performers->size(); ++i) {
		auto entity = performers->item_at(i);
		if (!entity)
			continue;

		if (entity->entity_type() != ZstEntityType::PERFORMER)
			continue;

		if (auto performer_wrapper = GetWrapper(entity->URI()))
			performer_wrappers.Add(static_cast<AShowtimePerformer*>(performer_wrapper));
	}

	return performer_wrappers;
}

AShowtimePerformer* UShowtimeClient::GetRootPerformer() const
{
	auto wrapper = GetWrapper(Handle()->get_root()->URI());
	if (wrapper) {
		return static_cast<AShowtimePerformer*>(wrapper);
	}
	return nullptr;
}

void UShowtimeClient::ConnectCable(AShowtimePlug* InputPlug, AShowtimePlug* OutputPlug) const
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

AShowtimeEntity* UShowtimeClient::SpawnEntity(ZstEntityBase* entity)
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

AShowtimePerformer* UShowtimeClient::SpawnPerformer(ZstPerformer* performer)
{
	// Create a new performer actor from our template performer
	if (auto entity_actor = GetWorld()->SpawnActor<AShowtimePerformer>(SpawnablePerformer)) {
		RegisterSpawnedWrapper(entity_actor, performer);
		return entity_actor;
	}
	return nullptr;
}

AShowtimeComponent* UShowtimeClient::SpawnComponent(ZstComponent* component) 
{
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

AShowtimeCable* UShowtimeClient::SpawnCable(ZstCable* cable)
{
	if (auto cable_actor = GetWorld()->SpawnActor<AShowtimeCable>(SpawnableCable)) {
		CableWrappers.Add(cable->get_address(), cable_actor);
		cable_actor->OwningClient = this;
		return cable_actor;
	}
	return nullptr;
}

AShowtimeServerBeacon* UShowtimeClient::SpawnServerBeacon(const ZstServerAddress* server)
{
	if (auto server_beacon_actor = GetWorld()->SpawnActor<AShowtimeServerBeacon>(SpawnableServer)) {
		server_beacon_actor->OwningClient = this;
		server_beacon_actor->Server = FServerAddressFromShowtime(server);
		ServerBeaconWrappers.Add(server_beacon_actor->Server, server_beacon_actor);
		return server_beacon_actor;
	}
	return nullptr;
}

AShowtimeFactory* UShowtimeClient::SpawnFactory(ZstEntityFactory* factory)
{
	if (auto factory_actor = GetWorld()->SpawnActor<AShowtimeFactory>(SpawnableFactory)){
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

AShowtimePlug* UShowtimeClient::SpawnPlug(ZstPlug* plug) 
{
	//auto transform = FTransform();
	//auto plug_actor =  Cast<AActor>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, SpawnablePlug->GetClass(), transform));
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

void UShowtimeClient::RegisterSpawnedWrapper(AShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->init(this, entity_path);
	EntityWrappers.Add(entity_path, wrapper);
}

AShowtimeEntity* UShowtimeClient::GetWrapperParent(const AShowtimeEntity* wrapper) const
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

AShowtimeEntity* UShowtimeClient::GetWrapper(const ZstEntityBase* entity) const
{
	return *EntityWrappers.Find(UTF8_TO_TCHAR(entity->URI().path()));
}

AShowtimeEntity* UShowtimeClient::GetWrapper(const ZstURI& URI) const
{
	return *EntityWrappers.Find(UTF8_TO_TCHAR(URI.path()));
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
