#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstServerAddress.h>
#include <showtime/ZstLogging.h>

namespace showtime {

	class ZST_CLASS_EXPORTED ZstLogAdaptor
#ifndef SWIG
		: public inheritable_enable_shared_from_this< ZstLogAdaptor >
#endif
	{
	public:
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, log_record, const Log::Record*, record)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, formatted_log_record, const char*, record)
		
		ZST_EXPORT virtual ~ZstLogAdaptor() {};
	};
}