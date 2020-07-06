#pragma once
#include <showtime/ZstCable.h>
#include <showtime/ZstCableAddress.h>

#include "CoreMinimal.h"
//#include "GameFramework/Actor.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeCable.generated.h"

using namespace showtime;
/**
 *
 */


UCLASS()
class UShowtimeCableAddress : public UObject, public ZstCableAddress {
	GENERATED_BODY()
};


UCLASS()
class AShowtimeCable : public AActor {
	GENERATED_BODY()
public:
	UPROPERTY()
	UShowtimeCableAddress* Address;
};
