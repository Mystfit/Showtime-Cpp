#include "plugin.h"

namespace showtime {
	std::shared_ptr<CoreEntities> showtime::CoreEntities::create()
	{
		return std::make_shared<CoreEntities>();
	}

	void showtime::CoreEntities::init(const char* root_name)
	{
	}

	const char* showtime::CoreEntities::name()
	{
		return PLUGIN_NAME;
	}

	int showtime::CoreEntities::version_major()
	{
		return PLUGIN_MAJOR_VER;
	}

	int showtime::CoreEntities::version_minor()
	{
		return PLUGIN_MINOR_VER;
	}

	int showtime::CoreEntities::version_patch()
	{
		return PLUGIN_PATCH_VER;
	}

	void showtime::CoreEntities::get_factories(showtime::ZstEntityBundle& bundle)
	{
	}
}