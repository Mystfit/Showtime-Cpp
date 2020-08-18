#pragma once

#include <boost/dll/alias.hpp>
#include <showtime/ZstPlugin.h>
#include <showtime/ZstExports.h>

#define PLUGIN_MAJOR_VER 0
#define PLUGIN_MINOR_VER 0
#define PLUGIN_PATCH_VER 1
#define PLUGIN_NAME "rtmidi"

namespace showtime {

	class ZST_CLASS_EXPORTED RtMidiPlugin : public ZstPlugin {
	public:
		ZST_PLUGIN_EXPORT RtMidiPlugin();
		ZST_PLUGIN_EXPORT virtual ~RtMidiPlugin();

		ZST_PLUGIN_EXPORT static std::shared_ptr<RtMidiPlugin> create();
		ZST_PLUGIN_EXPORT virtual void init() override;
		ZST_PLUGIN_EXPORT virtual const char* name() override;
		ZST_PLUGIN_EXPORT virtual int version_major() override;
		ZST_PLUGIN_EXPORT virtual int version_minor() override;
		ZST_PLUGIN_EXPORT virtual int version_patch() override;
	};
}

BOOST_DLL_ALIAS(showtime::RtMidiPlugin::create, create_plugin);
