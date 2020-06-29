#pragma once

#include <showtime/ZstServerAddress.h>
#include "CoreMinimal.h"
#include "ServerAddress.generated.h"

USTRUCT(BlueprintType)
struct FServerAddress {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime Server Address")
	FString name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime Server Address")
	FString address;
};

static FServerAddress FServerAddressFromShowtime(const showtime::ZstServerAddress& server_address) {
	FServerAddress address;
	address.name = UTF8_TO_TCHAR(server_address.name.c_str());
	address.address = UTF8_TO_TCHAR(server_address.address.c_str());
	return address;
}