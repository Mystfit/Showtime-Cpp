// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include <Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ServerAddress.h"
#include "ClientAdaptors.h"
#include "ShowtimeServerBeacon.h"
#include <ShowtimeView.h>
#include "ShowtimeClient.generated.h"

using namespace showtime;


// Forward declarations
// --------------------
namespace showtime {
	class ShowtimeClient;
}

// Log categories
// --------------
DECLARE_LOG_CATEGORY_EXTERN(Showtime, Display, All);


// Enum mappings
// -------------

// static TMap<showtime::Log::Level, ELogVerbosity::Type> LogLevel_to_ELogVerbosity = {
// 	{ showtime::Log::Level::debug, ELogVerbosity::Type::Log },
// 	{ showtime::Log::Level::notification, ELogVerbosity::Type::Log },
// 	{ showtime::Log::Level::warn, ELogVerbosity::Type::Warning },
// 	{ showtime::Log::Level::error, ELogVerbosity::Type::Error }
// };


// Event delegates
// --------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FConnectedToServer, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDisconnectedFromServer, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerDiscovered, UShowtimeClient*, Client, AShowtimeServerBeacon*, ServerBeacon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerLost, UShowtimeClient*, Client, AShowtimeServerBeacon*, ServerBeacon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGraphSynchronised, UShowtimeClient*, Client, FServerAddress, ServerAddress);


//// Plugin delegates

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPluginLoaded, std::shared_ptr<ZstPlugin>, plugin)
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPluginUnloaded, std::shared_ptr<ZstPlugin>, plugin)


UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeClient : public UActorComponent
{
	GENERATED_BODY()
public:

	UShowtimeClient();
	void Cleanup();
	

	// Properties
	// ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Showtime|Client")
	FString ClientName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Showtime|Client")
	TMap<FServerAddress, AShowtimeServerBeacon*> ServerBeaconWrappers;




	// BP functions
	// ------------

	UFUNCTION(BlueprintCallable, Exec, Category="Showtime|Client")
	void Init();

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	void JoinServerByName(const FString& name);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	void LeaveServer();

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	bool IsConnected() const;
	
	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	TArray<AShowtimePerformer*> GetPerformers() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	AShowtimePerformer* GetRootPerformer() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	void ConnectCable(AShowtimePlug* InputPlug, AShowtimePlug* OutputPlug) const;


	//UPROPERTY(BlueprintAssignable)
	//FPluginLoaded OnPluginLoaded;

	//UPROPERTY(BlueprintAssignable)
	//FPluginUnloaded OnPluginUnloaded;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FConnectedToServer OnConnectedToServer;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FDisconnectedFromServer OnDisconnectedFromServer;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FServerDiscovered OnServerDiscovered;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FServerLost OnServerLost;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FGraphSynchronised OnGraphSynchronised;

	AShowtimeServerBeacon* SpawnServerBeacon(const ZstServerAddress* server);


	// Native access
	// --------------

	// Native handle to the Showtime|Client
	TSharedPtr<ShowtimeClient> Handle() const;

	UPROPERTY(BlueprintReadWrite, Category = "Showtime|Client")
	UShowtimeView* View;


	// Actor overrides
	// ---------------

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	void AttachEvents();
	void RemoveEvents();
	TSharedPtr<showtime::ShowtimeClient> client;
	std::shared_ptr<ClientAdaptors> client_adaptor;
};
