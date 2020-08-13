#pragma once

#include <showtime/ZstURI.h>
#include <showtime/entities/ZstEntityBase.h>

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeEntity.generated.h"

using namespace showtime;

// Forwards
class UShowtimeClient;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeEntity : public UActorComponent {
	GENERATED_BODY()
public:
	void init(UShowtimeClient* owner, FString entity_path);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Entity")
	UShowtimeEntity* GetParent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime Entity")
	UShowtimeClient* OwningClient;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime Entity")
	TArray<UShowtimeEntity*> GetChildren(bool recursive = false);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime Entity")
	FString EntityPath;

	ZstEntityBase* GetNativeEntity();
};
