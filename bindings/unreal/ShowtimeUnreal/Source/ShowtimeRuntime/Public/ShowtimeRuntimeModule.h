#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include <showtime/Showtime.h>

DECLARE_LOG_CATEGORY_EXTERN(Showtime, Log, All);
DEFINE_LOG_CATEGORY(Showtime);

class FShowtimeRuntimeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FShowtimeRuntimeModule, ShowtimeRuntime);

