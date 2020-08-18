#include "MidiFactory.h"

MidiFactory::MidiFactory(const char* name) : 
	showtime::ZstEntityFactory(name)
{
	//this->add_creatable<MidiPort>("midi_port");
	this->add_creatable<MidiPort>("midi_port", [](const char* e_name) -> std::unique_ptr<ZstEntityBase> { return std::make_unique<MidiPort>(e_name); });
}
