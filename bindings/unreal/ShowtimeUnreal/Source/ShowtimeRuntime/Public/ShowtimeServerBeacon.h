#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ServerAddress.h"
#include "ShowtimeServerBeacon.generated.h"

using namespace showtime;
/**
 *
 */
UCLASS()
class AShowtimeServerBeacon : public AActor {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Showtime|Client")
	FServerAddress Server;
};
