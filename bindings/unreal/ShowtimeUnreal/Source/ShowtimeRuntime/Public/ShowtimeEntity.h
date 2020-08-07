#pragma once

#include <showtime/ZstURI.h>
#include <showtime/entities/ZstEntityBase.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeEntity.generated.h"

using namespace showtime;

// Forwards
class UShowtimeClient;
/**
 *
 */
UCLASS()
class AShowtimeEntity : public AActor {
	GENERATED_BODY()
public:
	void init(UShowtimeClient* owner, FString entity_path);

	UShowtimeClient* Owner;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime Entity")
	FString EntityPath;
};
