#pragma once

#include <showtime/ZstURI.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeURI.generated.h"

using namespace showtime;

/**
 *
 */
UCLASS()
class UShowtimeURI : public UStruct, public ZstURI {
	GENERATED_BODY()
public:
	UShowtimeURI();
	UShowtimeURI(const ZstURI& uri);
};
