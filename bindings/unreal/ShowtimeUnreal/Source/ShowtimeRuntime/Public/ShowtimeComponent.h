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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FComponentPlaced, UShowtimeComponent*, component);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlugPlaced, UShowtimePlug*, plug);


UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent, DisplayName="Showtime Component"))
class UShowtimeComponent : public UShowtimeEntity {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintNativeEvent, Category= "Showtime|Component")
	void PlugPlaced(UShowtimePlug* plug);
	void PlugPlaced_Implementation(UShowtimePlug* plug);

	UFUNCTION(BlueprintNativeEvent, Category = "Showtime|Component")
	void ComponentPlaced(UShowtimeComponent* component);
	void ComponentPlaced_Implementation(UShowtimeComponent* component);

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FComponentPlaced OnComponentPlaced;

	UPROPERTY(BlueprintAssignable, Category = "Showtime|Entity")
	FPlugPlaced OnPlugPlaced;

	//UFUNCTION(BlueprintCallable, Category = "Showtime|Component")
	//void AttachPlug(AShowtimePlug* plug);

	/*UFUNCTION(BlueprintCallable, Category = "Showtime|Component")
	void AttachComponent(UShowtimeComponent* component);*/

	ZstComponent* GetNativeComponent() const;
};
