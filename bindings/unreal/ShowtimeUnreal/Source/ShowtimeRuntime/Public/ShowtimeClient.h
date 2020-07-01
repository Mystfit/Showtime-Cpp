// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/Showtime.h>
//#include <Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ServerAddress.h"
#include "ShowtimeClient.generated.h"

using namespace showtime;


DECLARE_LOG_CATEGORY_EXTERN(Showtime, Display, All);

static TMap<Log::Level, ELogVerbosity::Type> LogLevel_to_ELogVerbosity = {
	{ Log::Level::debug, ELogVerbosity::Type::Log },
	{ Log::Level::notification, ELogVerbosity::Type::Log },
	{ Log::Level::warn, ELogVerbosity::Type::Warning },
	{ Log::Level::error, ELogVerbosity::Type::Error }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FConnectedToServer, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDisconnectedFromServer, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerDiscovered, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerLost, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGraphSynchronised, UShowtimeClient*, Client, FServerAddress, ServerAddress);


UCLASS(ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeClient : public UActorComponent, public ShowtimeClient {
	GENERATED_BODY()
public:
	friend class FClientLogAdaptor;
	friend class FClientConnectionAdaptor;

	UShowtimeClient();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Showtime Client")
	FString ClientName;

	UFUNCTION(BlueprintCallable, Exec, Category="Showtime Client")
	void Init();

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Client")
	void JoinServerByName(const FString& name);

	//UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Client")
	//void JoinServerByAddress(const FIPv4Address& address);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Client")
	void LeaveServer();

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// Event delegates
	// ---------------
	
	UPROPERTY(BlueprintAssignable)
	FConnectedToServer OnConnectedToServer;
	
	UPROPERTY(BlueprintAssignable)
	FDisconnectedFromServer OnDisconnectedFromServer;

	UPROPERTY(BlueprintAssignable)
	FServerDiscovered OnServerDiscovered;

	UPROPERTY(BlueprintAssignable)
	FServerLost OnServerLost;

	UPROPERTY(BlueprintAssignable)
	FGraphSynchronised OnGraphSynchronised;

	
private:
	std::shared_ptr<FClientLogAdaptor> LoggerAdaptor;
	std::shared_ptr<FClientConnectionAdaptor> ConnectionAdaptor;
};


class FClientLogAdaptor : public ZstLogAdaptor {
public:
	virtual void on_log_record(const Log::Record& record) override;
};


class FClientConnectionAdaptor : public ZstConnectionAdaptor {
public:
	FClientConnectionAdaptor(UShowtimeClient* client);
	virtual void on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress& server) override;
	virtual void on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress& server) override;
	virtual void on_server_discovered(ShowtimeClient* client, const ZstServerAddress& server) override;
	virtual void on_server_lost(ShowtimeClient* client, const ZstServerAddress& server) override;
	virtual void on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress& server) override;

private:
	UPROPERTY()
	UShowtimeClient* OwningClient;
};
