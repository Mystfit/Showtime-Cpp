// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeClient.h"
#include <functional>

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient() : client_adaptor(std::make_shared<ClientAdaptors>(this))
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShowtimeClient::Init()
{
	this->init(TCHAR_TO_UTF8(*ClientName), true);
}

void UShowtimeClient::JoinServerByName(const FString& name)
{
	this->auto_join_by_name_async(TCHAR_TO_UTF8(*name));
}

//void UShowtimeClient::JoinServerByAddress(const FIPv4Address& address)
//{
//	auto ip = address.ToText().ToString();
//	this->join(TCHAR_TO_UTF8(*ip));
//}

void UShowtimeClient::LeaveServer()
{
	this->leave();
}

void UShowtimeClient::BeginPlay()
{
	Super::BeginPlay();
	AttachEvents();
}

void UShowtimeClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	RemoveEvents();
	this->destroy();
}

void UShowtimeClient::AttachEvents(){
	this->add_connection_adaptor(client_adaptor);
	this->add_log_adaptor(client_adaptor);
}

void UShowtimeClient::RemoveEvents(){	this->remove_connection_adaptor(client_adaptor);
	this->remove_connection_adaptor(client_adaptor);
	this->remove_log_adaptor(client_adaptor);
}

void UShowtimeClient::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	this->poll_once();
}
