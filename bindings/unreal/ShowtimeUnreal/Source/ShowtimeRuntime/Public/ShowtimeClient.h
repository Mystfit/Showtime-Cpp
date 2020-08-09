// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/ShowtimeClient.h>
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

DECLARE_LOG_CATEGORY_EXTERN(Showtime, Display, All);

static TMap<showtime::Log::Level, ELogVerbosity::Type> LogLevel_to_ELogVerbosity = {
	{ showtime::Log::Level::debug, ELogVerbosity::Type::Log },
	{ showtime::Log::Level::notification, ELogVerbosity::Type::Log },
	{ showtime::Log::Level::warn, ELogVerbosity::Type::Warning },
	{ showtime::Log::Level::error, ELogVerbosity::Type::Error }
};


// Connection delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FConnectedToServer, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDisconnectedFromServer, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerDiscovered, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerLost, UShowtimeClient*, Client, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGraphSynchronised, UShowtimeClient*, Client, FServerAddress, ServerAddress);


// Hierarchy delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerArriving, AShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerLeaving, const UShowtimeURI*, performer_path);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityArriving, AShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityLeaving, const UShowtimeURI*, entity_path);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityUpdated, AShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryArriving, AShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryLeaving, const UShowtimeURI*, factory_path);


// Session delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableCreated, AShowtimeCable*, cable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableDestroyed, const UShowtimeCableAddress*, cable_address);


//// Plugin delegates
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPluginLoaded, std::shared_ptr<ZstPlugin>, plugin)
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPluginUnloaded, std::shared_ptr<ZstPlugin>, plugin)


// Forward declarations
class MulticastAndroid;


UCLASS(Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeClient : public UActorComponent
{
	GENERATED_BODY()
public:

	UShowtimeClient();
	void Cleanup();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Showtime Client")
	FString ClientName;

	UFUNCTION(BlueprintCallable, Exec, Category="Showtime Client")
	void Init();

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Client")
	void JoinServerByName(const FString& name);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Client")
	void LeaveServer();

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Client")
	bool IsConnected();
	
	// UE4 ZST Graphical wrappers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime Client")
	TMap<FString, AShowtimeEntity*> EntityWrappers;


	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// Actor prototypes
	// ----------------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime Client")
	TSubclassOf<AShowtimePerformer> SpawnablePerformer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime Client")
	TSubclassOf<AShowtimeComponent> SpawnableComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime Client")
	TSubclassOf<AShowtimePlug> SpawnablePlug;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime Client")
	TSubclassOf<AShowtimeFactory> SpawnableFactory;


	// Event delegates
	// ---------------
	
	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FConnectedToServer OnConnectedToServer;
	
	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FDisconnectedFromServer OnDisconnectedFromServer;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FServerDiscovered OnServerDiscovered;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FServerLost OnServerLost;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FGraphSynchronised OnGraphSynchronised;

	// ---

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FPerformerArriving OnPerformerArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FPerformerLeaving OnPerformerLeaving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FEntityArriving OnEntityArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FEntityUpdated OnEntityUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FFactoryArriving OnFactoryArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FFactoryLeaving OnFactoryLeaving;

	// ---

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FCableCreated OnCableCreated;

	UPROPERTY(BlueprintAssignable, Category = "Showtime Client")
	FCableDestroyed OnCableLeaving;

	// ---

	//UPROPERTY(BlueprintAssignable)
	//FPluginLoaded OnPluginLoaded;

	//UPROPERTY(BlueprintAssignable)
	//FPluginUnloaded OnPluginUnloaded;

	TSharedPtr<ShowtimeClient>& Handle();


	// Spawning wrappers
	void RefreshEntityWrappers();
	AShowtimeEntity* SpawnEntity(ZstEntityBase* entity);
	AShowtimePerformer* SpawnPerformer(ZstPerformer* performer);
	AShowtimeComponent* SpawnComponent(ZstComponent* component);
	AShowtimePlug* SpawnPlug(ZstPlug* plug);
	void RegisterSpawnedWrapper(AShowtimeEntity* wrapper, ZstEntityBase* entity); \

	// Wrapper managements
	AShowtimeEntity* GetWrapperParent(AShowtimeEntity* wrapper);


private:
	void AttachEvents();
	void RemoveEvents();
#if PLATFORM_ANDROID
	TSharedPtr<MulticastAndroid> multicast_manager;
#endif
	TSharedPtr<ShowtimeClient> client;
	std::shared_ptr<ClientAdaptors> client_adaptor;
};
