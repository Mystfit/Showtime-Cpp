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
class AShowtimeEntity : public AActor {
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
	AShowtimeEntity* GetParent() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	TArray<AShowtimeEntity*> GetChildren(bool recursive = false) const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	FString GetName() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Entity")
	void AddChild(AShowtimeEntity* entity);


	// Events
	// ------

	UPROPERTY(BlueprintAssignable, Replicated, Category = "Showtime|Client")
	FInitialised OnInitialised;


	// Native functions


	void init(UShowtimeClient* owner, FString entity_path);
	ZstEntityBase* GetNativeEntity() const;
};


FORCEINLINE uint32 GetTypeHash(const AShowtimeEntity* Other)
{
	return GetTypeHash(Other->EntityPath);
}
