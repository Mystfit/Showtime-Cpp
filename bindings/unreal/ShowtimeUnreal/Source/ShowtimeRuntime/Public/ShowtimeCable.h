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

	bool operator==(const FShowtimeCableAddress& s) const
	{
		return std::tie(InputPath, OutputPath) == std::tie(s.InputPath, s.OutputPath);
	}
};

FORCEINLINE uint32 GetTypeHash(const FShowtimeCableAddress& Other)
{
	return GetTypeHash(Other.InputPath) ^ GetTypeHash(Other.OutputPath);
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Showtime|Cable")
	UShowtimeClient* OwningClient;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Cable")
	FShowtimeCableAddress Address() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Cable")
	UShowtimePlug* GetInputPlug() const;

	UFUNCTION(BlueprintCallable, Exec, Category = "Showtime|Cable")
	UShowtimePlug* GetOutputPlug() const;

	ZstCable* GetNativeCable() const;
};
