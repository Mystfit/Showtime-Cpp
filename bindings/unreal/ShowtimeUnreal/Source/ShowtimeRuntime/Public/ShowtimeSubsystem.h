// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include <Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h>
#include "CoreMinimal.h"
#include "Tickable.h"
#include "Components/ActorComponent.h"
#include "ServerAddress.h"
#include "ClientAdaptors.h"
#include "ShowtimeView.h"
#include "ShowtimeServerBeacon.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "ShowtimeSubsystem.generated.h"

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConnectedToServer, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDisconnectedFromServer, FServerAddress, ServerAddress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGraphSynchronised, FServerAddress, ServerAddress);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerDiscovered, UShowtimeClient*, Client, AShowtimeServerBeacon*, ServerBeacon);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerLost, UShowtimeClient*, Client, AShowtimeServerBeacon*, ServerBeacon);

//// Plugin delegates

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPluginLoaded, std::shared_ptr<ZstPlugin>, plugin)
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPluginUnloaded, std::shared_ptr<ZstPlugin>, plugin)


UCLASS(Abstract, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	//UShowtimeClient(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin Subsystem interface

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// End subsystem interface


	//UShowtimeClient();
	void Cleanup();
	

	// Properties
	// ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Showtime|Client")
	FString ClientName;



	// BP functions
	// ------------

	UFUNCTION(BlueprintCallable, Exec, Category="Showtime|Client")
	void Init(TSubclassOf<class UShowtimeView> ViewClassToSpawn);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Client")
	void JoinServerByAddress(const FString& address);

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
	void ConnectCable(UShowtimeCable* pending_cable, UShowtimePlug* InputPlug, UShowtimePlug* OutputPlug) const;


	//UPROPERTY(BlueprintAssignable)
	//FPluginLoaded OnPluginLoaded;

	//UPROPERTY(BlueprintAssignable)
	//FPluginUnloaded OnPluginUnloaded;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FConnectedToServer OnConnectedToServer;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FDisconnectedFromServer OnDisconnectedFromServer;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FGraphSynchronised OnGraphSynchronised;


	// Native access
	// --------------

	// Native handle to the Showtime|Client
	TSharedPtr<ShowtimeClient> Handle() const;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Showtime|View")
	UShowtimeView* View;


	// Actor overrides
	// ---------------
	virtual void PostInitProperties() override; 
	
	UFUNCTION(BlueprintNativeEvent)
	void BeginPlay();
	void BeginPlay_Implementation();
	
	UFUNCTION(BlueprintNativeEvent)
	void Tick(float DeltaTime) override;
	void Tick_Implementation(float DeltaTime);

	virtual bool IsAllowedToTick() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual TStatId GetStatId() const override;

private:
	void AttachEvents();
	void RemoveEvents();
	void AttachView(UShowtimeView* NewView);
	void RemoveView();

	TSharedPtr<showtime::ShowtimeClient> client;
	std::shared_ptr<ClientAdaptors> client_adaptor;
};
