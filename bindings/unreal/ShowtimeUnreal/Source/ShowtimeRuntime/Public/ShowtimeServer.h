// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/ShowtimeServer.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeServer.generated.h"

#define DEFAULT_SHOWTIME_SERVER "stage"

using namespace showtime;

/**
 *
 */
UCLASS()
class UShowtimeServer : public UObject, public ShowtimeServer {
	GENERATED_BODY()
public:
	virtual void BeginDestroy() override;
};
