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
class UShowtimeComponent : public UShowtimeEntity {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent, Category= "Showtime|Component")
	void PlugAttached(UShowtimePlug* plug);

	UFUNCTION(BlueprintImplementableEvent, Category = "Showtime|Component")
	void ComponentAttached(UShowtimeComponent* component);

	UFUNCTION(BlueprintCallable, Category = "Showtime|Component")
	void AttachPlug(UShowtimePlug* plug);

	UFUNCTION(BlueprintCallable, Category = "Showtime|Component")
	void AttachComponent(UShowtimeComponent* component);

	ZstComponent* GetNativeComponent() const;
};
