#pragma once

#include <showtime/ZstExports.h>
#include <showtime/ZstPlugin.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>

namespace showtime {

	class ZST_CLASS_EXPORTED ZstPluginAdaptor :
		public ZstEventAdaptor
	{
	public:
		ZST_EXPORT virtual void on_plugin_loaded(std::shared_ptr<ZstPlugin> plugin);
		ZST_EXPORT virtual void on_plugin_unloaded(std::shared_ptr<ZstPlugin> plugin);
	};

}
