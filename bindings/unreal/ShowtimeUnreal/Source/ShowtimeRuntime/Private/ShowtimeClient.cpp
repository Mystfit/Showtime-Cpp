// Fill out your copyright notice in the Description page of Project Settings.
#include "ShowtimeClient.h"
#include "ShowtimePerformer.h"
#include "ShowtimeView.h"
#include <showtime/ShowtimeClient.h>

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#ifdef PLATFORM_ANDROID
#include "MulticastAndroid.h"
#include "ShowtimePluginManagerAndroid.h"
#endif
#include <functional>

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient(const FObjectInitializer& ObjectInitializer) : 
	client(MakeShared<ShowtimeClient>()),
	m_shouldTick(false)
{
	//View = static_cast<UShowtimeView*>(CreateDefaultSubobject("View", ViewClass, ViewClass, /*bIsRequired =*/ true, false));
}

void UShowtimeClient::Cleanup()
{
	RemoveEvents();
#if PLATFORM_ANDROID
	UMulticastAndroid::ReleaseMulticastLock();
#endif
	client->destroy();
}

void UShowtimeClient::Init()
{
	FString plugin_path;

#if PLATFORM_ANDROID
	UMulticastAndroid::AcquireMulticastLock();
	plugin_path = UShowtimePluginManagerAndroid::GetPluginPath();
#endif
	if(!plugin_path.IsEmpty())
		client->set_plugin_path(TCHAR_TO_UTF8(*plugin_path));
	client->init(TCHAR_TO_UTF8(*ClientName), true);

	client->add_connection_adaptor(View);	// For server beacons
	client->add_session_adaptor(View);      // For cables
	client->add_hierarchy_adaptor(View);	// For entities
	//client->start_file_logging();

	View->SpawnPerformer(Handle()->get_root());
}

void UShowtimeClient::JoinServerByAddress(const FString& address)
{
	client->join_async(TCHAR_TO_UTF8(*address));
}

void UShowtimeClient::JoinServerByName(const FString& name)
{
	client->auto_join_by_name_async(TCHAR_TO_UTF8(*name));
}

void UShowtimeClient::LeaveServer()
{
	client->leave();
}

bool UShowtimeClient::IsConnected() const
{
	return client->is_connected();
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

		if (auto performer_wrapper = View->GetWrapper(entity->URI()))
			performer_wrappers.Add(static_cast<AShowtimePerformer*>(performer_wrapper));
	}

	return performer_wrappers;
}

AShowtimePerformer* UShowtimeClient::GetRootPerformer() const
{
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



void UShowtimeClient::PostInitProperties()
{
	Super::PostInitProperties(); 

	// Create view from class
	if (!ViewClass)
		ViewClass = UShowtimeView::StaticClass();
	FString name;
	ViewClass->GetName(name);
	View = NewObject<UShowtimeView>(this, ViewClass, FName(*name), RF_Transient);
	
	// Trigger blueprint beginplay
	if(GetOuter() && GetOuter()->GetWorld()) 
		BeginPlay();
}

void UShowtimeClient::BeginPlay_Implementation()
{
	AttachEvents();
}

void UShowtimeClient::BeginDestroy()
{
	Super::BeginDestroy();
	Cleanup();
}

TSharedPtr<ShowtimeClient> UShowtimeClient::Handle() const
{
	return client;
}

void UShowtimeClient::AttachEvents(){
	client_adaptor = std::make_shared<ClientAdaptors>(this);
	client->add_connection_adaptor(client_adaptor.get());
	//client->add_hierarchy_adaptor(client_adaptor);
	client->add_log_adaptor(client_adaptor.get());
}

void UShowtimeClient::RemoveEvents(){
	client->remove_connection_adaptor(client_adaptor.get());
	//client->remove_hierarchy_adaptor(client_adaptor);
	client->remove_log_adaptor(client_adaptor.get());
}

void UShowtimeClient::Tick_Implementation(float DeltaTime)
{
	client->poll_once();
}

bool UShowtimeClient::IsAllowedToTick() const
{
	return client->is_init_completed();
}

bool UShowtimeClient::IsTickable() const
{
	return client->is_init_completed();
}

bool UShowtimeClient::IsTickableInEditor() const
{
	return false;
}

bool UShowtimeClient::IsTickableWhenPaused() const
{
	return true;
}

TStatId UShowtimeClient::GetStatId() const
{
	return UObject::GetStatID();
}
