#pragma once

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstServerAddress.h>

namespace showtime {

	//namespace Log {
	//	//Forward log record class to avoid dependency on ZstLogging.h
	//	struct Record;
	//}

	class ZST_CLASS_EXPORTED ZstLogAdaptor
#ifndef SWIG
		: public inheritable_enable_shared_from_this< ZstLogAdaptor >
#endif
	{
	public:
		//MULTICAST_DELEGATE_OneParam(ZST_EXPORT, log_record, const Log::Record*, record)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, formatted_log_record, const char*, record)
	};
}
