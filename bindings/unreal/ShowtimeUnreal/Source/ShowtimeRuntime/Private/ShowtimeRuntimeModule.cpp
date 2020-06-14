#include "ShowtimeRuntimeModule.h"
#include <showtime/Showtime.h>

void FShowtimeRuntimeModule::StartupModule()
{
	UE_LOG(Showtime, Log, TEXT("################################### \n"));
	UE_LOG(Showtime, Log, TEXT("## Loading Showtime module... \n"));
	UE_LOG(Showtime, Log, TEXT("################################### \n"));
	//client.init("UE4_showtime_client", true);
	//client.auto_join_by_name("stage");

	//ZstLog::app(ZstLog::LogLevel::debug, "Created test Showtime client");
}


void FShowtimeRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	//ZstLog::app(ZstLog::LogLevel::debug, "Destroying test Showtime client");
	client.destroy();
}
