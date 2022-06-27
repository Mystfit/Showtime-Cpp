// Fill out your copyright notice in the Description page of Project Settings.
#include "ShowtimeSubsystem.h"
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
#include "..\Public\ShowtimeSubsystem.h"

DEFINE_LOG_CATEGORY(Showtime);

//UShowtimeSubsystem::UShowtimeSubsystem(const FObjectInitializer& ObjectInitializer) : 
//{
//	//View = static_cast<UShowtimeView*>(CreateDefaultSubobject("View", ViewClass, ViewClass, /*bIsRequired =*/ true, false));
//}

void UShowtimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	client = MakeShared<ShowtimeClient>();

	// Create view from class
	/*if (!ViewClass)
		ViewClass = UShowtimeView::StaticClass();
	FString name;
	ViewClass->GetName(name);
	View = NewObject<UShowtimeView>(this, ViewClass, FName(*name), RF_Transient);*/

	// Trigger blueprint beginplay
	if (GetOuter() && GetOuter()->GetWorld())
		BeginPlay();
}

void UShowtimeSubsystem::Deinitialize()
{
	RemoveEvents();
#if PLATFORM_ANDROID
	UMulticastAndroid::ReleaseMulticastLock();
#endif
	client->destroy();
}

void UShowtimeSubsystem::Cleanup()
{
}

void UShowtimeSubsystem::Init(TSubclassOf<class UShowtimeView> ViewClass)
{
	FString plugin_path;

	// Create view
	FString name;
	ViewClass->GetName(name);
	auto ViewInstance = NewObject<UShowtimeView>(this, ViewClass, FName(*name), RF_Transient);
	AttachView(ViewInstance);

#if PLATFORM_ANDROID
	UMulticastAndroid::AcquireMulticastLock();
	plugin_path = UShowtimePluginManagerAndroid::GetPluginPath();
#endif
	if(!plugin_path.IsEmpty())
		client->set_plugin_path(TCHAR_TO_UTF8(*plugin_path));
	client->init(TCHAR_TO_UTF8(*ClientName), true);

	if (!View) {
		UE_LOG(Showtime, Error, TEXT("No Showtime view class set"));
		return;
	}

	// Additional event adaptors
	AttachEvents();

	View->SpawnEntity(Handle()->get_root());
}

void UShowtimeSubsystem::JoinServerByAddress(const FString& address)
{
	client->join_async(TCHAR_TO_UTF8(*address));
}

void UShowtimeSubsystem::JoinServerByName(const FString& name)
{
	client->auto_join_by_name_async(TCHAR_TO_UTF8(*name));
}

void UShowtimeSubsystem::LeaveServer()
{
	client->leave();
}

bool UShowtimeSubsystem::IsConnected() const
{
	return client->is_connected();
}

TArray<UShowtimePerformer*> UShowtimeSubsystem::GetPerformers() const
{
	TArray<UShowtimePerformer*> performer_wrappers = TArray<UShowtimePerformer*>();

	auto performers = std::make_shared<ZstEntityBundle>();
	Handle()->get_performers(performers.get());

	for (int i = 0; i < performers->size(); ++i) {
		auto entity = performers->item_at(i);
		if (!entity)
			continue;

		if (entity->entity_type() != ZstEntityType::PERFORMER)
			continue;

		if (auto performer_wrapper = View->GetWrapper(entity->URI()))
			performer_wrappers.Add(static_cast<UShowtimePerformer*>(performer_wrapper));
	}

	return performer_wrappers;
}

UShowtimePerformer* UShowtimeSubsystem::GetRootPerformer() const
{
	auto wrapper = View->GetWrapper(Handle()->get_root()->URI());
	if (wrapper) {
		return static_cast<UShowtimePerformer*>(wrapper);
	}
	return nullptr;
}

void UShowtimeSubsystem::ConnectCable(UShowtimeCable* pending_cable, UShowtimePlug* InputPlug, UShowtimePlug* OutputPlug) const
{
	if (!InputPlug || !OutputPlug) {
		UE_LOG(Showtime, Display, TEXT("Input or Outplug plug was null"));
		return;
	}

	auto cable = Handle()->connect_cable(static_cast<ZstInputPlug*>(InputPlug->GetNativePlug()), static_cast<ZstOutputPlug*>(OutputPlug->GetNativePlug()));
	if (cable) {
		pending_cable->Address = FShowtimeCableAddress(cable->get_address());
	}
}



void UShowtimeSubsystem::PostInitProperties()
{
	Super::PostInitProperties(); 
}

void UShowtimeSubsystem::BeginPlay_Implementation()
{
}

//void UShowtimeSubsystem::BeginDestroy()
//{
//	Super::BeginDestroy();
//	Cleanup();
//}

TSharedPtr<ShowtimeClient> UShowtimeSubsystem::Handle() const
{
	return client;
}

void UShowtimeSubsystem::AttachEvents(){
	client_adaptor = std::make_shared<ClientAdaptors>(this);
	//client->add_connection_adaptor(client_adaptor.get());
	//client->add_hierarchy_adaptor(client_adaptor);
	client->add_log_adaptor(client_adaptor.get());
}

void UShowtimeSubsystem::RemoveEvents(){
	//client->remove_connection_adaptor(client_adaptor.get());
	//client->remove_hierarchy_adaptor(client_adaptor);
	client->remove_log_adaptor(client_adaptor.get());
}

void UShowtimeSubsystem::AttachView(UShowtimeView* NewView)
{
	if (!client)
		return;

	RemoveView();
	View = NewView;

	client->add_connection_adaptor(View);	// For server beacons
	client->add_session_adaptor(View);      // For cables
	client->add_hierarchy_adaptor(View);	// For entities
}

void UShowtimeSubsystem::RemoveView()
{
	if (!client)
		return;

	if (View) {
		if (client->is_init_completed()) {
			client->remove_connection_adaptor(View);	// For server beacons
			client->remove_session_adaptor(View);      // For cables
			client->remove_hierarchy_adaptor(View);	// For entities
		}
		View->ConditionalBeginDestroy();
	}
}

void UShowtimeSubsystem::Tick_Implementation(float DeltaTime)
{
	client->poll_once();
}

bool UShowtimeSubsystem::IsAllowedToTick() const
{
	return client->is_init_completed();
}

bool UShowtimeSubsystem::IsTickable() const
{
	if (client) {
		return client->is_init_completed();
	}
	return false;
}

bool UShowtimeSubsystem::IsTickableInEditor() const
{
	return false;
}

bool UShowtimeSubsystem::IsTickableWhenPaused() const
{
	return true;
}

TStatId UShowtimeSubsystem::GetStatId() const
{
	return UObject::GetStatID();
}
