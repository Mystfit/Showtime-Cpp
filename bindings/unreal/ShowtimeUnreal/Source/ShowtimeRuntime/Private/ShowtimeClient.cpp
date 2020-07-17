// Fill out your copyright notice in the Description page of Project Settings.
#include "ShowtimeClient.h"

#ifdef PLATFORM_ANDROID
#include "MulticastAndroid.h"
#endif
#include <functional>

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient()
{
	PrimaryComponentTick.bCanEverTick = true;
#if PLATFORM_ANDROID
	MulticastAndroid::InitMulticastFunctions();
#endif
}

void UShowtimeClient::Cleanup()
{
	RemoveEvents();
#if PLATFORM_ANDROID
	MulticastAndroid::ReleaseMulticastLock();
#endif
	if (client) client->destroy();
}

void UShowtimeClient::Init()
{
	if (client) client->init(TCHAR_TO_UTF8(*ClientName), true);
#if PLATFORM_ANDROID
	MulticastAndroid::AcquireMulticastLock();
#endif
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

void UShowtimeClient::AttachEvents(){
	if (!client)
		return;

	client_adaptor = std::make_shared<ClientAdaptors>(this);
	client->add_connection_adaptor(client_adaptor);
	client->add_log_adaptor(client_adaptor);
}

void UShowtimeClient::RemoveEvents(){
	if (!client)
		return;

	client->remove_connection_adaptor(client_adaptor);
	client->remove_log_adaptor(client_adaptor);
}

void UShowtimeClient::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if(client) client->poll_once();
}
