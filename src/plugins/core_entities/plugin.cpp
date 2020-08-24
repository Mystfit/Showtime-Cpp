#include "plugin.h"

namespace showtime {
	CoreEntities::CoreEntities() : ZstPlugin()
	{
		std::unique_ptr<ZstEntityFactory> math_factory = std::make_unique<MathEntityFactory>("math_entities");
		add_factory(math_factory);
	}

	CoreEntities::~CoreEntities()
	{
	}

	std::shared_ptr<CoreEntities> showtime::CoreEntities::create()
	{
		return std::make_shared<CoreEntities>();
	}

	void showtime::CoreEntities::init(const char* plugin_data_path)
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
}