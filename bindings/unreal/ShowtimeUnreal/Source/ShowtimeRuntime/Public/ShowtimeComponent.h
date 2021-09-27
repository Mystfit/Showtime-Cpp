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
UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent, DisplayName="Showtime Component"))
class AShowtimeComponent : public AShowtimeEntity {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent, Category= "Showtime|Component")
	void PlugAttached(AShowtimePlug* plug);

	UFUNCTION(BlueprintImplementableEvent, Category = "Showtime|Component")
	void ComponentAttached(AShowtimeComponent* component);

	UFUNCTION(BlueprintCallable, Category = "Showtime|Component")
	void AttachPlug(AShowtimePlug* plug);

	UFUNCTION(BlueprintCallable, Category = "Showtime|Component")
	void AttachComponent(AShowtimeComponent* component);

	ZstComponent* GetNativeComponent() const;
};
