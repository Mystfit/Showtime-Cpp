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

	if (!View)
		return;

	auto performers = std::make_shared<ZstEntityBundle>();
	Handle()->get_performers(performers.get());

	for (int i = 0; i < performers->size(); ++i) {
		auto entity = performers->item_at(i);
		if (!entity)
			continue;

		if (entity->entity_type() != ZstEntityType::PERFORMER)
			continue;

		if (auto performer_wrapper = View->GetWrapper(entity->URI()))
			performer_wrappers.Add(static_cast<AShowtimePerformer*>(performer_wrapper));
	}

	return performer_wrappers;
}

AShowtimePerformer* UShowtimeClient::GetRootPerformer() const
{
	if (!View)
		return;

	auto wrapper = View->GetWrapper(Handle()->get_root()->URI());
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
