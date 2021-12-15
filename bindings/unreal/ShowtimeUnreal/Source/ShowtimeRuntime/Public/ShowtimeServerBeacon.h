#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ServerAddress.h"
#include "ShowtimeServerBeacon.generated.h"

using namespace showtime;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimeServerBeacon : public UActorComponent {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Showtime|Client")
	FServerAddress Server;
};
