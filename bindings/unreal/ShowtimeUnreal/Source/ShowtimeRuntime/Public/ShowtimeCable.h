#pragma once
#include <showtime/ZstCable.h>
#include <showtime/ZstCableAddress.h>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/NoExportTypes.h"
#include "ShowtimePlug.h"

#include "ShowtimeCable.generated.h"

// Forwards
class UShowtimeClient;

using namespace showtime;

USTRUCT(Blueprintable)
struct FShowtimeCableAddress {
	GENERATED_BODY()
	FString InputPath;
	FString OutputPath;

	FShowtimeCableAddress() {};
	FShowtimeCableAddress(const ZstCableAddress& cable_address) :
		InputPath(UTF8_TO_TCHAR(cable_address.get_input_URI().path())),
		OutputPath(UTF8_TO_TCHAR(cable_address.get_output_URI().path())) {}
	FShowtimeCableAddress(const ZstURI& input_path, const ZstURI& output_path) :
		InputPath(UTF8_TO_TCHAR(input_path.path())),
		OutputPath(UTF8_TO_TCHAR(output_path.path())){}

	bool operator==(const FShowtimeCableAddress& s) const
	{
		return std::tie(InputPath, OutputPath) == std::tie(s.InputPath, s.OutputPath);
	}
};

FORCEINLINE uint32 GetTypeHash(const FShowtimeCableAddress& Other)
{
	return GetTypeHash(Other.InputPath) ^ GetTypeHash(Other.OutputPath);
}

FORCEINLINE ZstCableAddress CableAddressFromUnreal(const FShowtimeCableAddress& address) {
	return ZstCableAddress(ZstURI(TCHAR_TO_UTF8(*address.InputPath)), ZstURI(TCHAR_TO_UTF8(*address.OutputPath)));
}


//
//UENUM(BlueprintType)
//enum UCableGhostStatus {
//	CableGhostStatus_Valid,
//	CableGhostStatus_Invalid,
//	CableGhostStatus_Ghosting,
//	CableGhostStatus_WaitingForServer,
//	CableGhostStatus_Active
//};


UCLASS()
class AShowtimeCable : public AActor {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Showtime|Cable")
	UShowtimeClient* OwningClient;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Showtime|Cable")
	FShowtimeCableAddress Address;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Cable")
	AShowtimePlug* GetInputPlug() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Cable")
	AShowtimePlug* GetOutputPlug() const;

	ZstCable* GetNativeCable() const;

	bool operator==(const AShowtimeCable& s) const
	{
		return std::tie(Address) == std::tie(s.Address);
	}
};

FORCEINLINE uint32 GetTypeHash(const AShowtimeCable& Other)
{
	return GetTypeHash(Other.Address);
}
