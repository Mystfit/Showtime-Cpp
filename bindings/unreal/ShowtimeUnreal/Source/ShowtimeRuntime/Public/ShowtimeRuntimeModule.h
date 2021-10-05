#pragma once

#include <showtime/ShowtimeClient.h>
#include <memory>

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FShowtimeRuntimeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	FShowtimeRuntimeModule* GetModule() const;
	ShowtimeClient* GetClient() const;

private:
	std::shared_ptr<ShowtimeClient> m_client;
};

IMPLEMENT_MODULE(FShowtimeRuntimeModule, ShowtimeRuntime);

