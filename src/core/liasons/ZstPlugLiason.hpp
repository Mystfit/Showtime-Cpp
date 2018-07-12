#pragma once

#include <ZstExports.h>
#include <ZstCable.h>
#include <entities/ZstPlug.h>
#include "../ZstValue.h"

class ZstPlugLiason {
public:
	ZST_EXPORT void plug_remove_cable(ZstPlug * plug, ZstCable * cable);
	ZST_EXPORT void plug_add_cable(ZstPlug * plug, ZstCable * cable);
};