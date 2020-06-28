// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/ShowtimeServer.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeServer.generated.h"

using namespace showtime;

#define DEFAULT_SHOWTIME_SERVER "stage"

/**
 *
 */
UCLASS()
class UShowtimeServer : public UObject, public ShowtimeServer {
	GENERATED_BODY()
public:
	virtual void BeginDestroy() override;
};
