#pragma once

#include <showtime/entities/ZstPlug.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ShowtimeEntity.h"
#include "ShowtimePlug.generated.h"

using namespace showtime;

UENUM(BlueprintType)
enum UShowtimePlugDirection {
	Direction_None,
	Direction_In,
	Direction_Out
};
static TMap<ZstPlugDirection, UShowtimePlugDirection> ZstPlugDirection_to_ShowtimePlugDirection = {
	{ ZstPlugDirection::IN_JACK, UShowtimePlugDirection::Direction_In },
	{ ZstPlugDirection::OUT_JACK, UShowtimePlugDirection::Direction_Out },
	{ ZstPlugDirection::NONE, UShowtimePlugDirection::Direction_None }
};


UENUM(BlueprintType)
enum UShowtimeValueType {
	ValueType_None,
	ValueType_IntList,
	ValueType_FloatList,
	ValueType_StrList
};

static TMap<ZstValueType, UShowtimeValueType> ZstValueType_to_UShowtimeValueType = {
	{ ZstValueType::IntList, UShowtimeValueType::ValueType_IntList },
	{ ZstValueType::FloatList, UShowtimeValueType::ValueType_FloatList },
	{ ZstValueType::StrList, UShowtimeValueType::ValueType_StrList },
	{ ZstValueType::NONE, UShowtimeValueType::ValueType_None }
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = (Showtime), meta = (BlueprintSpawnableComponent))
class UShowtimePlug : public UShowtimeEntity {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Plug")
	UShowtimePlugDirection Direction() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Plug")
	UShowtimeValueType ValueType() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Plug")
	void SetIntValues(TArray<int> values);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Plug")
	void SetFloatValues(TArray<float> values);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Plug")
	void SetStringValues(TArray<FString> values);

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Plug")
	void Fire();

	ZstPlug* GetNativePlug() const;
};
