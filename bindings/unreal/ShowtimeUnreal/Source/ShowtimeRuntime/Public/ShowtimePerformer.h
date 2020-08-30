#pragma once

#include <showtime/entities/ZstPerformer.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ShowtimeComponent.h"
#include "ShowtimePerformer.generated.h"

using namespace showtime;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimePerformer : public UShowtimeComponent {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Showtime|Factory")
	void FactoryAttached(UShowtimeFactory* factory);
};
