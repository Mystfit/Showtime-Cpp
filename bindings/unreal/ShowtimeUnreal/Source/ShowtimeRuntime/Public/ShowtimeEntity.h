#pragma once

#include <showtime/ZstURI.h>
#include <showtime/entities/ZstEntityBase.h>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeEntity.generated.h"

using namespace showtime;

// Forwards
class UShowtimeClient;

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInitialised);


UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeEntity : public UActorComponent {
	GENERATED_BODY()
public:
	// Properties
	// ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime|Entity")
	UShowtimeClient* OwningClient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Showtime|Entity")
	FString EntityPath;


	// Blueprint callable functions
	// ----------------------------

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	UShowtimeEntity* GetParent() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	TArray<UShowtimeEntity*> GetChildren(bool recursive = false) const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	FString GetName() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	void AddChild(UShowtimeEntity* entity);


	// Events
	// ------

	UPROPERTY(BlueprintAssignable, Replicated, Category = "Showtime|Client")
	FInitialised OnInitialised;


	// Native functions


	void init(UShowtimeClient* owner, FString entity_path);
	ZstEntityBase* GetNativeEntity() const;
};
