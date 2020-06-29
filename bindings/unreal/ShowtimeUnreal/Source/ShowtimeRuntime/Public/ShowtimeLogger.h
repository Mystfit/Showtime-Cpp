#pragma once

#include <showtime/Showtime.h>
#include "CoreMinimal.h"

using namespace showtime;

DECLARE_LOG_CATEGORY_EXTERN(Showtime, Display, All);

static TMap<Log::Level, ELogVerbosity::Type> LogLevel_to_ELogVerbosity = {
	{ Log::Level::debug, ELogVerbosity::Type::Log },
	{ Log::Level::notification, ELogVerbosity::Type::Log },
	{ Log::Level::warn, ELogVerbosity::Type::Warning },
	{ Log::Level::error, ELogVerbosity::Type::Error }
};

class ShowtimeLogger : public ZstLogAdaptor  {
public:
	void on_log_record(const Log::Record& record) override;
};
