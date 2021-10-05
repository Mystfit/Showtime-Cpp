#include "Modules/ModuleManager.h"
#include "ShowtimeRuntimeModule.h"

void FShowtimeRuntimeModule::StartupModule()
{
	UE_LOG(Showtime, Log, TEXT("################################### \n"));
	UE_LOG(Showtime, Log, TEXT("## Loading Showtime module... \n"));
	UE_LOG(Showtime, Log, TEXT("################################### \n"));

	m_client = std::make_shared<ShowtimeClient>();
}


void FShowtimeRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	m_client->destroy();
}

FShowtimeRuntimeModule* FShowtimeRuntimeModule::GetModule() const
{
	return FModuleManager::Get().GetModulePtr<FShowtimeRuntimeModule>("ShowtimeRuntimeModule");
}

ShowtimeClient* FShowtimeRuntimeModule::GetClient() const
{
	return m_client.get();
}
