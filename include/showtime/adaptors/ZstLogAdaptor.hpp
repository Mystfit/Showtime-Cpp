#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstServerAddress.h>
#include <showtime/ZstLogging.h>

namespace showtime {

	class ZST_CLASS_EXPORTED ZstLogAdaptor :
		public ZstEventAdaptor<ZstLogAdaptor>
	{
	public:
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, log_record, const Log::Record*, record)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, formatted_log_record, const char*, record)
	};
}