#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstPlugin.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {

	class ZST_CLASS_EXPORTED ZstPluginAdaptor
#ifndef SWIG
		: public inheritable_enable_shared_from_this< ZstPluginAdaptor >
#endif
	{
	public:
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, plugin_loaded, ZstPlugin*, plugin)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, plugin_unloaded, ZstPlugin*, plugin)
	};

}
