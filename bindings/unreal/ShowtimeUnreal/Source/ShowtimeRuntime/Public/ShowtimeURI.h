#pragma once

#include <showtime/ZstURI.h>

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "ShowtimeURI.generated.h"

using namespace showtime;

/**
 *
 */
UCLASS(Blueprintable)
class UShowtimeURI : public UObject{
	GENERATED_BODY()

public:

	//UShowtimeURI() : m_wrapped_URI() {};
	//UShowtimeURI(const ZstURI& URI) : m_wrapped_URI(URI) 
	void Init(const FName& URI);
	void Init(const ZstURI& URI);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Showtime|URI")
	FName Path;

	ZstURI Native() const;

	virtual bool IsSupportedForNetworking() const override { return true; };
	void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const;
};
