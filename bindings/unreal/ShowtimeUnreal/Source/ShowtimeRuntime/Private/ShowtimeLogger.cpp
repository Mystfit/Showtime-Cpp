#include "ShowtimeLogger.h"

DEFINE_LOG_CATEGORY(Showtime);

void ShowtimeLogger::on_log_record(const Log::Record& record)
{
	FString message(record.message.c_str());
	UE_LOG(Showtime, Display, TEXT("%s"), *message);
}
