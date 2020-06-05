#pragma once

#include <boost/dll/alias.hpp>
#include "ZstPlugin.h"
#include "ZstExports.h"

#define PLUGIN_MAJOR_VER 0
#define PLUGIN_MINOR_VER 0
#define PLUGIN_PATCH_VER 1
#define PLUGIN_NAME "core_entities"

namespace showtime {

	class ZST_CLASS_EXPORTED CoreEntities : public ZstPlugin {
	public:
		ZST_PLUGIN_EXPORT static std::shared_ptr<CoreEntities> create();
		ZST_PLUGIN_EXPORT virtual void init(const char* root_name);
		ZST_PLUGIN_EXPORT virtual const char* name();
		ZST_PLUGIN_EXPORT virtual int version_major();
		ZST_PLUGIN_EXPORT virtual int version_minor();
		ZST_PLUGIN_EXPORT virtual int version_patch();
		ZST_PLUGIN_EXPORT virtual void get_factories(showtime::ZstEntityBundle& bundle);

	private:
		std::vector< std::unique_ptr<showtime::ZstEntityFactory> > m_factories;
	};
}

BOOST_DLL_ALIAS(showtime::CoreEntities::create, create_plugin);
