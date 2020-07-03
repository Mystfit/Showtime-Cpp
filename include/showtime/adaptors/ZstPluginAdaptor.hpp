#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstPlugin.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {

	class ZST_CLASS_EXPORTED ZstPluginAdaptor :
		public ZstEventAdaptor
	{
	public:
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, plugin_loaded, std::shared_ptr<ZstPlugin>, plugin)
		MULTICAST_DELEGATE_OneParam(ZST_EXPORT, plugin_unloaded, std::shared_ptr<ZstPlugin>, plugin)
	};

}
