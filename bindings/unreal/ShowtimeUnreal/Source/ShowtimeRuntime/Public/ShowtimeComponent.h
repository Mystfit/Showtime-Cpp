#pragma once

#include <showtime/ZstURI.h>
#include <showtime/entities/ZstComponent.h>
#include <showtime/entities/ZstPlug.h>
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ShowtimeEntity.h"
#include "ShowtimePlug.h"
#include "ShowtimeComponent.generated.h"

using namespace showtime;
/**
 *
 */
UCLASS()
class AShowtimeComponent : public AShowtimeEntity {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent, Category="Showtime Component")
	void AttachPlug(AShowtimePlug* plug);

	UFUNCTION(BlueprintImplementableEvent, Category = "Showtime Component")
	void AttachComponent(AShowtimeComponent* component);

	ZstComponent* GetNative();
};
