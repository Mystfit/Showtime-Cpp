// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include <showtime/adaptors/ZstHierarchyAdaptor.hpp>

#include "ShowtimeCable.h"
#include "ShowtimeURI.h"
#include "ShowtimePerformer.h"
#include "ShowtimeEntity.h"
#include "ShowtimeComponent.h"
#include "ShowtimePlug.h"
#include "ShowtimeCable.h"
#include "ShowtimeFactory.h"
#include "ShowtimeURI.h"

#include "CoreMinimal.h"
#include "ShowtimeView.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerArriving, AShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerLeaving, AShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityArriving, AShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityLeaving, AShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityUpdated, AShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryArriving, AShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryLeaving, AShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableCreated, AShowtimeCable*, cable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableDestroyed, AShowtimeCable*, cable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerDiscovered, UShowtimeClient*, Client, AShowtimeServerBeacon*, ServerBeacon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FServerLost, UShowtimeClient*, Client, AShowtimeServerBeacon*, ServerBeacon);


UCLASS(Blueprintable)
class SHOWTIMERUNTIME_API UShowtimeView : 
	public UObject, 
	public ZstSessionAdaptor, 
	public ZstHierarchyAdaptor,
	public ZstConnectionAdaptor
{
	GENERATED_BODY()
public:
	// Showtime events
	// ---------------
	void on_performer_arriving(showtime::ZstPerformer* performer) override;
	void on_performer_leaving(const showtime::ZstURI& performer_path) override;
	void on_entity_arriving(showtime::ZstEntityBase* entity) override;
	void on_entity_leaving(const showtime::ZstURI& entity_path) override;
	void on_entity_updated(showtime::ZstEntityBase* entity) override;
	void on_factory_arriving(showtime::ZstEntityFactory* factory) override;
	void on_factory_leaving(const showtime::ZstURI& factory_path) override;
	void on_cable_created(showtime::ZstCable* cable) override;
	void on_cable_destroyed(const showtime::ZstCableAddress& cable_address) override;
	void on_server_discovered(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	void on_server_lost(showtime::ShowtimeClient* client, const showtime::ZstServerAddress* server) override;
	//void on_plugin_loaded(std::shared_ptr<ZstPlugin> plugin) override;
	//void on_plugin_unloaded(std::shared_ptr<ZstPlugin> plugin) override;


	AShowtimeEntity* SpawnEntity(ZstEntityBase* entity);

	UFUNCTION(BlueprintNativeEvent)
	AShowtimePerformer* SpawnPerformer(const UShowtimeURI* path);

	UFUNCTION(BlueprintNativeEvent)
	AShowtimeComponent* SpawnComponent(const UShowtimeURI* path);

	//UFUNCTION(BlueprintImplementableEvent)
	AShowtimeCable* SpawnCable(ZstCable* cable);

	UFUNCTION(BlueprintNativeEvent)
	AShowtimeFactory* SpawnFactory(const UShowtimeURI* path);

	UFUNCTION(BlueprintNativeEvent)
	AShowtimePlug* SpawnPlug(const UShowtimeURI* path);


	AShowtimeServerBeacon* SpawnServerBeacon(const ZstServerAddress* server);

	// Wrappers
	// ----------------

	void RegisterSpawnedWrapper(AShowtimeEntity* wrapper, const UShowtimeURI* path);

	// Wrapper management
	AShowtimeEntity* GetWrapperParent(const AShowtimeEntity* wrapper) const;
	AShowtimeEntity* GetWrapper(const ZstEntityBase* entity) const;
	AShowtimeEntity* GetWrapper(const ZstURI& URI) const;


	// Actor prototypes
	// ----------------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AShowtimePerformer> SpawnablePerformer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AShowtimeComponent> SpawnableComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AShowtimePlug> SpawnablePlug;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AShowtimeFactory> SpawnableFactory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AShowtimeServerBeacon> SpawnableServer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AShowtimeCable> SpawnableCable;


	// Event delegates
	// ---------------

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FPerformerArriving OnPerformerArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FPerformerLeaving OnPerformerLeaving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FEntityArriving OnEntityArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FEntityUpdated OnEntityUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FEntityLeaving OnEntityLeaving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FFactoryArriving OnFactoryArriving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FFactoryLeaving OnFactoryLeaving;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FCableCreated OnCableCreated;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FCableDestroyed OnCableDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FServerDiscovered OnServerDiscovered;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Client")
	FServerLost OnServerLost;


	UPROPERTY(BlueprintReadOnly, Category = "Showtime|Entity")
	TMap<FString, AShowtimeEntity*> EntityWrappers;

	UPROPERTY(BlueprintReadOnly, Category = "Showtime|Entity")
	TMap<FString, AShowtimePerformer*> PerformerWrappers;

	UPROPERTY(BlueprintReadOnly, Category = "Showtime|Entity")
	TMap<FShowtimeCableAddress, AShowtimeCable*> CableWrappers;

	UPROPERTY(BlueprintReadOnly, Category = "Showtime|Client")
	TMap<FServerAddress, AShowtimeServerBeacon*> ServerBeaconWrappers;

private:
	UShowtimeClient* GetOwner() const;
};
