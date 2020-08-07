// Fill out your copyright notice in the Description page of Project Settings.
#include "ShowtimeClient.h"

#ifdef PLATFORM_ANDROID
#include "MulticastAndroid.h"
#endif
#include <functional>
#include "..\Public\ShowtimeClient.h"

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient() 
#if PLATFORM_ANDROID
	: multicast_manager(MakeShared<MulticastAndroid>())
#endif
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShowtimeClient::Cleanup()
{
	RemoveEvents();
#if PLATFORM_ANDROID
	multicast_manager->ReleaseMulticastLock();
#endif
	if (client) client->destroy();
}

void UShowtimeClient::Init()
{
#if PLATFORM_ANDROID
	multicast_manager->InitMulticastFunctions();
	multicast_manager->AcquireMulticastLock();
#endif
	if (client) client->init(TCHAR_TO_UTF8(*ClientName), true);
}

void UShowtimeClient::JoinServerByName(const FString& name)
{
	if (client) client->auto_join_by_name_async(TCHAR_TO_UTF8(*name));
}

void UShowtimeClient::LeaveServer()
{
	if (client) client->leave();
}

bool UShowtimeClient::IsConnected()
{
	return (client) ? client->is_connected() : false;
}

void UShowtimeClient::BeginPlay()
{
	Super::BeginPlay();
	client = MakeShared <ShowtimeClient>();
	AttachEvents();
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

TSharedPtr<ShowtimeClient>& UShowtimeClient::Handle()
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
	default:
		break;
	}

	return nullptr;
}

AShowtimePerformer* UShowtimeClient::SpawnPerformer(ZstPerformer* performer)
{
	
	auto entity_wrapper = GetWorld()->SpawnActor<AShowtimePerformer>(SpawnablePerformer);
	RegisterSpawnedWrapper(entity_wrapper, performer);
	return entity_wrapper;
}

AShowtimeComponent* UShowtimeClient::SpawnComponent(ZstComponent* component) 
{
	auto entity_wrapper = GetWorld()->SpawnActor<AShowtimeComponent>(SpawnableComponent);
	RegisterSpawnedWrapper(entity_wrapper, component);
	return entity_wrapper;
}

AShowtimePlug* UShowtimeClient::SpawnPlug(ZstPlug* plug) 
{
	auto entity_wrapper = GetWorld()->SpawnActor<AShowtimePlug>(SpawnablePlug);
	RegisterSpawnedWrapper(entity_wrapper, plug);
	return entity_wrapper;
}

void UShowtimeClient::RegisterSpawnedWrapper(AShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->init(this, entity_path);
	EntityWrappers.Add(entity_path, wrapper);
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
