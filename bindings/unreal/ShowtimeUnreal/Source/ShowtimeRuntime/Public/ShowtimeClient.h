// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <showtime/Showtime.h>

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeClient.generated.h"

using namespace showtime;

/**
 * 
 */
UCLASS()
class UShowtimeClient : public UObject, public ShowtimeClient, public FTickableGameObject {
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
};
