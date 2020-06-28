#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstServerAddress.h>
#include <showtime/ZstLogging.h>

namespace showtime {

	class ZST_CLASS_EXPORTED ZstLogAdaptor :
		public ZstEventAdaptor
	{
	public:
		ZST_EXPORT virtual void on_log_record(const Log::Record& record);
	};
}