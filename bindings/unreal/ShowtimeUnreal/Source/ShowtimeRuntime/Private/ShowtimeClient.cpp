// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeClient.h"
#include <functional>

DEFINE_LOG_CATEGORY(Showtime);

UShowtimeClient::UShowtimeClient()
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

void UShowtimeClient::AttachEvents(){
	this->connection_events()->connected_to_server() += [this](ShowtimeClient* client, const ZstServerAddress& server){ 
		OnConnectedToServer.Broadcast(this, FServerAddressFromShowtime(server));
	};

	this->connection_events()->disconnected_from_server() += [this](ShowtimeClient* client, const ZstServerAddress& server){
		OnDisconnectedFromServer.Broadcast(this, FServerAddressFromShowtime(server));
	};

	this->connection_events()->server_discovered() += [this](ShowtimeClient* client, const ZstServerAddress& server){ 
		OnServerDiscovered.Broadcast(this, FServerAddressFromShowtime(server));
	};

	this->connection_events()->server_lost() += [this](ShowtimeClient* client, const ZstServerAddress& server){ 
		OnServerLost.Broadcast(this, FServerAddressFromShowtime(server));
	};

	this->connection_events()->synchronised_graph() += [this](ShowtimeClient* client, const ZstServerAddress& server){ 
		OnGraphSynchronised.Broadcast(this, FServerAddressFromShowtime(server));
	};

	this->log_events()->log_record() += std::bind(&UShowtimeClient::OnLogRecord, this, std::placeholders::_1);
}

void UShowtimeClient::RemoveEvents(){
}


void UShowtimeClient::BeginDestroy()
{
	Super::BeginDestroy();
	this->destroy();
}

void UShowtimeClient::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	this->poll_once();
}


void UShowtimeClient::OnLogRecord(const Log::Record& record)
{
	FString message(record.message.c_str());
	UE_LOG(Showtime, Display, TEXT("%s"), *message);
}