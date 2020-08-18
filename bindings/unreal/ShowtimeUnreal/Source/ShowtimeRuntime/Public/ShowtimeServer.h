// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeServer.generated.h"

#define DEFAULT_SHOWTIME_SERVER "stage"

// Forwards
namespace showtime {
	class ShowtimeServer;
}

using namespace showtime;


/**
 *
 */
UCLASS()
class UShowtimeServer : public UObject{
	GENERATED_BODY()
public:
	virtual void BeginDestroy() override;
	TSharedPtr<showtime::ShowtimeServer>& Handle();

private:
	TSharedPtr<showtime::ShowtimeServer> server;
};
