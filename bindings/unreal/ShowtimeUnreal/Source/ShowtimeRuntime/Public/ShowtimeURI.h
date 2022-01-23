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
	void Init(const ZstURI& URI);

	//UPROPERTY(BlueprintReadOnly, Category = "Showtime|URI")
	UFUNCTION(BlueprintCallable, Category = "Showtime|URI")
	int32 Size() const;

	UFUNCTION(BlueprintCallable, Category = "Showtime|URI")
	int32 FullSize() const;

	UFUNCTION(BlueprintCallable, Category = "Showtime|URI")
	FName Path() const;

	const ZstURI& WrappedURI() const;

private:
	ZstURI m_wrapped_URI;
};
