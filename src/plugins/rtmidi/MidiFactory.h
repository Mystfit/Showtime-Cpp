#pragma once

#include <showtime/ZstExports.h>
#include <showtime/entities/ZstEntityFactory.h>
#include <showtime/ZstURI.h>
#include "MidiPort.h"


class ZST_CLASS_EXPORTED MidiFactory : public showtime::ZstEntityFactory
{
public:
	MidiFactory(const char* name);
};
