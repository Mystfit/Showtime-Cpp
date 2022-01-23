// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include <showtime/adaptors/ZstConnectionAdaptor.hpp>
#include <showtime/adaptors/ZstHierarchyAdaptor.hpp>
#include <showtime/ShowtimeClient.h>

#include "ShowtimeCable.h"
#include "ShowtimeURI.h"
#include "ShowtimePerformer.h"
#include "ShowtimeEntity.h"
#include "ShowtimeComponent.h"
#include "ShowtimePlug.h"
#include "ShowtimeCable.h"
#include "ShowtimeFactory.h"

#include "CoreMinimal.h"
#include "ShowtimeView.generated.h"



// Forward declarations

class UShowtimeServerBeacon;


// Event declarations

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerArriving, UShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPerformerLeaving, UShowtimePerformer*, performer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityArriving, UShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityLeaving, UShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEntityUpdated, UShowtimeEntity*, entity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryArriving, UShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFactoryLeaving, UShowtimeFactory*, factory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableCreated, UShowtimeCable*, cable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCableDestroyed, UShowtimeCable*, cable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerDiscovered, UShowtimeServerBeacon*, ServerBeacon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FServerLost, UShowtimeServerBeacon*, ServerBeacon);


UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime))
class SHOWTIMERUNTIME_API UShowtimeView : 
	public UObject, 
	public ZstSessionAdaptor, 
	public ZstHierarchyAdaptor,
	public ZstConnectionAdaptor
{
	GENERATED_BODY()
public:
	UShowtimeView();

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


	// Spawners
	// --------

	
	UShowtimeEntity* SpawnEntity(ZstEntityBase* entity);
	
	template<typename wrapper_t>
	wrapper_t* SpawnEntityActorFromPrototype(ZstEntityBase* entity, TSubclassOf<AActor> prototype);

	//AActor* SpawnPerformer(ZstPerformer* performer);
	//AActor* SpawnComponent(ZstComponent* component);
	//AActor* SpawnFactory(ZstEntityFactory* factory);
	//AActor* SpawnPlug(ZstPlug* plug);
	UShowtimeCable* SpawnCable(ZstCable* cable);
	UShowtimeServerBeacon* SpawnServerBeacon(const FServerAddress& address);
	
	UFUNCTION(BlueprintNativeEvent)
	void PlaceEntity(UShowtimeEntity* entity);
	void PlaceEntity_Implementation(UShowtimeEntity* entity);

	UFUNCTION(BlueprintNativeEvent)
	void PlaceCable(UShowtimeCable* cable);
	void PlaceCable_Implementation(UShowtimeCable* cable);


	// Wrapper utilities
	// ----------------

	void RegisterSpawnedWrapper(UShowtimeEntity* wrapper, ZstEntityBase* entity);

	// Wrapper management
	//UShowtimeEntity* GetWrapperParent(const UShowtimeEntity* wrapper) const;
	UShowtimeEntity* GetWrapper(const ZstEntityBase* entity) const;
	UShowtimeEntity* GetWrapper(const ZstURI& URI) const;


	// Actor prototypes
	// ----------------

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AActor> SpawnablePerformer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AActor> SpawnableComponent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AActor> SpawnablePlug;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AActor> SpawnableFactory;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AActor> SpawnableServer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Entity")
	TSubclassOf<AActor> SpawnableCable;


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
	TMap<FName, UShowtimeEntity*> EntityWrappers;

	UPROPERTY(BlueprintReadOnly, Category = "Showtime|Entity")
	TMap<FName, UShowtimePerformer*> PerformerWrappers;

	UPROPERTY(BlueprintReadOnly, Category = "Showtime|Entity")
	TMap<FShowtimeCableAddress, UShowtimeCable*> CableWrappers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Showtime|Client")
	TMap<FServerAddress, UShowtimeServerBeacon*> ServerBeaconWrappers;
};

template<typename wrapper_t>
inline wrapper_t* UShowtimeView::SpawnEntityActorFromPrototype(ZstEntityBase* entity, TSubclassOf<AActor> prototype)
{
	auto world = GetWorld();
	if (!world)
		return nullptr;

	// Create a new performer actor from our template performer
	FActorSpawnParameters params;
	params.Name = UTF8_TO_TCHAR(entity->URI().path());
	params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	if (auto entity_actor = world->SpawnActor<AActor>(prototype, params)) {
		entity_actor->SetActorLabel(UTF8_TO_TCHAR(entity->URI().last().path()));
		wrapper_t* entity_comp = entity_actor->FindComponentByClass<wrapper_t>();
		if (!entity_comp) {
			entity_comp = NewObject<wrapper_t>(entity_actor);
			entity_comp->RegisterComponent();
		}
		RegisterSpawnedWrapper(entity_comp, entity);
		return entity_comp;
	}
	return nullptr;
}
