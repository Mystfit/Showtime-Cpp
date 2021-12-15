// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShowtimeEntity.h"
#include <showtime/entities/ZstEntityBase.h>

/**
 * 
 */
class SHOWTIMERUNTIME_API ShowtimeConversions
{
public:
	static TArray<UShowtimeEntity*> EntityBundleToWrappers(const ZstEntityBundle* bundle);
};
