#include "ShowtimeURI.h"

void UShowtimeURI::Init(const FName& URI)
{
    Path = URI;
}

void UShowtimeURI::Init(const ZstURI& URI)
{
    Path = UTF8_TO_TCHAR(URI.path());
}

ZstURI UShowtimeURI::Native() const
{
    return ZstURI(TCHAR_TO_UTF8(*Path.ToString()));
}

void UShowtimeURI::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UShowtimeURI, Path);
}
