#pragma once

#include <showtime/ZstURI.h>
#include <showtime/entities/ZstEntityFactory.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ShowtimeEntity.h"
#include "ShowtimeFactory.generated.h"

using namespace showtime;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeFactory : public UShowtimeEntity {
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Showtime|Factory")
	TMap<FString, AActor*> SpawnableComponents;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Factory")
	void AddSpawnableComponent(AActor* spawnable_actor);

	ZstEntityFactory* GetNativeFactory() const;
};
