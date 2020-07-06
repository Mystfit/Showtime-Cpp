#pragma once

#include <showtime/ZstURI.h>
#include <showtime/entities/ZstEntityBase.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeEntity.generated.h"

using namespace showtime;
/**
 *
 */
UCLASS()
class AShowtimeEntity : public AActor {
	GENERATED_BODY()
public:
private:
	ZstURI m_entity_path;
};
