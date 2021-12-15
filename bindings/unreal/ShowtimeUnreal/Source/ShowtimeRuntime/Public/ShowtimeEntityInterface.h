#pragma once
#include "ShowtimeEntity.h"
#include "ShowtimeEntityInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UShowtimeEntityInferface : public UInterface
{
    GENERATED_BODY()
};

class IShowtimeEntityInferface
{
    GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Showtime|Interfaces")
    UShowtimeEntity* Entity();
};
