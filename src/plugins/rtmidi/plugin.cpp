#include "plugin.h"
#include "MidiFactory.h"

namespace showtime {
	RtMidiPlugin::RtMidiPlugin() : ZstPlugin()
	{
		std::unique_ptr<ZstEntityFactory> midi_factory = std::make_unique<MidiFactory>("midi_ports");
		add_factory(midi_factory);
	}

	RtMidiPlugin::~RtMidiPlugin()
	{
	}

	std::shared_ptr<RtMidiPlugin> showtime::RtMidiPlugin::create()
	{
		return std::make_shared<RtMidiPlugin>();
	}

	void showtime::RtMidiPlugin::init()
	{
	}

	const char* showtime::RtMidiPlugin::name()
	{
		return PLUGIN_NAME;
	}

	int showtime::RtMidiPlugin::version_major()
	{
		return PLUGIN_MAJOR_VER;
	}

	int showtime::RtMidiPlugin::version_minor()
	{
		return PLUGIN_MINOR_VER;
	}

	int showtime::RtMidiPlugin::version_patch()
	{
		return PLUGIN_PATCH_VER;
	}
}
