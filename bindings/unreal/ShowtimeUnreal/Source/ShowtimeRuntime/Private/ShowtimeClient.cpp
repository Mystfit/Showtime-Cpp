// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeClient.h"

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient() : 
	LoggerAdaptor(std::make_shared<FClientLogAdaptor>()),
	ConnectionAdaptor(std::make_shared<FClientConnectionAdaptor>(this))
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShowtimeClient::Init()
{
	this->add_log_adaptor(LoggerAdaptor);
	this->add_connection_adaptor(ConnectionAdaptor);
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
}

void UShowtimeClient::BeginDestroy()
{
	Super::BeginDestroy();s
	this->destroy();
}

void UShowtimeClient::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	this->poll_once();
}


FClientConnectionAdaptor::FClientConnectionAdaptor(UShowtimeClient* client) : OwningClient(client)
{
}

void FClientConnectionAdaptor::on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress& server)
{
	OwningClient->OnConnectedToServer.Broadcast(FServerAddressFromShowtime(server));
}

//void FClientConnectionAdaptor::on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress& server)
//{
//	OwningClient->DisconnectedFromServerEvent.Broadcast(server);
//}
//
//void FClientConnectionAdaptor::on_server_discovered(ShowtimeClient* client, const ZstServerAddress& server)
//{
//	OwningClient->ServerDiscoveredEvent.Broadcast(server);
//}
//
//void FClientConnectionAdaptor::on_server_lost(ShowtimeClient* client, const ZstServerAddress& server)
//{
//	OwningClient->ServerLostEvent.Broadcast(server);
//}
//
//void FClientConnectionAdaptor::on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress& server)
//{
//	OwningClient->GraphSynchronisedEvent.Broadcast(server);
//}


void FClientLogAdaptor::on_log_record(const Log::Record& record)
{
	FString message(record.message.c_str());
	UE_LOG(Showtime, Display, TEXT("%s"), *message);
}
