// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include <Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ServerAddress.h"
#include "ClientAdaptors.h"
#include "ShowtimeCable.h"
#include "ShowtimeURI.h"
#include "ShowtimePerformer.h"
#include "ShowtimeEntity.h"
#include "ShowtimeComponent.h"
#include "ShowtimePlug.h"
#include "ShowtimeCable.h"
#include "ShowtimeFactory.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerDiscovered, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerLost, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGraphSynchronised, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerArriving, UShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerLeaving, UShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityArriving, UShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityLeaving, UShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityUpdated, UShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryArriving, UShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryLeaving, UShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableCreated, AShowtimeCable*, cable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableDestroyed, AShowtimeCable*, cable);


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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime|Client")
	TMap<FString, UShowtimeEntity*> EntityWrappers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime|Client")
	TMap<FShowtimeCableAddress, AShowtimeCable*> CableWrappers;


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
	TArray<UShowtimePerformer*> GetPerformers() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	UShowtimePerformer* GetRootPerformer() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	void ConnectCable(UShowtimePlug* InputPlug, UShowtimePlug* OutputPlug) const;


	// Actor prototypes
	// ----------------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Client")
	TSubclassOf<AActor> SpawnablePerformer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Client")
	TSubclassOf<AActor> SpawnableComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Client")
	TSubclassOf<AActor> SpawnablePlug;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Client")
	TSubclassOf<AActor> SpawnableFactory;


	// Event delegates
	// ---------------
	
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

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FPerformerArriving OnPerformerArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FPerformerLeaving OnPerformerLeaving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FEntityArriving OnEntityArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FEntityUpdated OnEntityUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FFactoryArriving OnFactoryArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FFactoryLeaving OnFactoryLeaving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FCableCreated OnCableCreated;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FCableDestroyed OnCableDestroyed;

	//UPROPERTY(BlueprintAssignable)
	//FPluginLoaded OnPluginLoaded;

	//UPROPERTY(BlueprintAssignable)
	//FPluginUnloaded OnPluginUnloaded;


	// Native access
	// --------------

	// Native handle to the Showtime|Client
	TSharedPtr<ShowtimeClient> Handle() const;


	// Wrapper spawning
	// ----------------
	void RefreshEntityWrappers();
	UShowtimeEntity* SpawnEntity(ZstEntityBase* entity);
	UShowtimePerformer* SpawnPerformer(ZstPerformer* performer);
	UShowtimeComponent* SpawnComponent(ZstComponent* component);
	AShowtimeCable* SpawnCable(ZstCable* cable);
	UShowtimeFactory* SpawnFactory(ZstEntityFactory* factory);
	UShowtimePlug* SpawnPlug(ZstPlug* plug);
	void RegisterSpawnedWrapper(UShowtimeEntity* wrapper, ZstEntityBase* entity);

	// Wrapper management
	UShowtimeEntity* GetWrapperParent(const UShowtimeEntity* wrapper) const;


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
