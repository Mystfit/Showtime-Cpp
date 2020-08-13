// Fill out your copyright notice in the Description page of Project Settings.
#include "ShowtimeClient.h"
#include "ShowtimePerformer.h"

#ifdef PLATFORM_ANDROID
#include "MulticastAndroid.h"
#endif
#include <functional>

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient() : client(MakeShared<ShowtimeClient>())
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

TArray<UShowtimePerformer*> UShowtimeClient::GetPerformers()
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

UShowtimeEntity* UShowtimeClient::SpawnEntity(ZstEntityBase* entity)
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

	RegisterSpawnedWrapper(entity_wrapper, component);
	return entity_wrapper;
}

UShowtimePlug* UShowtimeClient::SpawnPlug(ZstPlug* plug) 
{
	auto entity_actor = GetWorld()->SpawnActor<AActor>(SpawnablePlug);
	if (!entity_actor)
		return nullptr;

	auto entity_wrapper = entity_actor->FindComponentByClass<UShowtimePlug>();

	if (!entity_wrapper)
		entity_wrapper = NewObject<UShowtimePlug>(entity_actor);

	RegisterSpawnedWrapper(entity_wrapper, plug);

	//Add plug to parent component
	if (auto parent = entity_wrapper->GetParent()) {
		if (parent->GetNativeEntity()->entity_type() == ZstEntityType::COMPONENT) {
			auto parent_c = static_cast<UShowtimeComponent*>(parent);
			parent_c->AttachPlug(entity_wrapper);
		}
	}
	return entity_wrapper;
}

void UShowtimeClient::RegisterSpawnedWrapper(UShowtimeEntity* wrapper, ZstEntityBase* entity)
{
	if (!wrapper)
		return;

	FString entity_path = UTF8_TO_TCHAR(entity->URI().path());
	wrapper->init(this, entity_path);
	EntityWrappers.Add(entity_path, wrapper);
}

UShowtimeEntity* UShowtimeClient::GetWrapperParent(UShowtimeEntity* wrapper)
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
