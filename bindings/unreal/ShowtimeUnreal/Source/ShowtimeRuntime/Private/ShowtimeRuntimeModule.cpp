#include "ShowtimeRuntimeModule.h"

void FShowtimeRuntimeModule::StartupModule()
{
	UE_LOG(Showtime, Log, TEXT("################################### \n"));
	UE_LOG(Showtime, Log, TEXT("## Loading Showtime module... \n"));
	UE_LOG(Showtime, Log, TEXT("################################### \n"));
}


void FShowtimeRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}
