#pragma once

#include <boost/dll/alias.hpp>
#include <showtime/ZstPlugin.h>
#include <showtime/ZstExports.h>
#include "MathEntityFactory.h"

#define PLUGIN_MAJOR_VER 0
#define PLUGIN_MINOR_VER 0
#define PLUGIN_PATCH_VER 1
#define PLUGIN_NAME "core_entities"

namespace showtime {

	class ZST_CLASS_EXPORTED CoreEntities : public ZstPlugin {
	public:
		ZST_PLUGIN_EXPORT CoreEntities();
		ZST_PLUGIN_EXPORT virtual ~CoreEntities();

		ZST_PLUGIN_EXPORT static std::shared_ptr<CoreEntities> create();
		ZST_PLUGIN_EXPORT virtual void init(const char* plugin_data_path) override;
		ZST_PLUGIN_EXPORT virtual const char* name() override;
		ZST_PLUGIN_EXPORT virtual int version_major() override;
		ZST_PLUGIN_EXPORT virtual int version_minor() override;
		ZST_PLUGIN_EXPORT virtual int version_patch() override;
	};
}

BOOST_DLL_ALIAS(showtime::CoreEntities::create, create_plugin);
