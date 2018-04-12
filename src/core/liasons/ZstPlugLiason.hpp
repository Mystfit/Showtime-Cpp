#pragma once

#include <ZstCable.h>
#include <entities/ZstPlug.h>

class ZstPlugLiason {
protected:
	void plug_remove_cable(ZstPlug * plug, ZstCable * cable);
	void plug_add_cable(ZstPlug * plug, ZstCable * cable);
};