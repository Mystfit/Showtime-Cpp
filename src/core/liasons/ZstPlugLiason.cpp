#include "ZstPlugLiason.hpp"

void ZstPlugLiason::plug_remove_cable(ZstPlug * plug, ZstCable * cable)
{
	plug->remove_cable(cable);
}

void ZstPlugLiason::plug_add_cable(ZstPlug * plug, ZstCable * cable)
{
	plug->add_cable(cable);
}

ZstValue * ZstPlugLiason::plug_raw_value(ZstPlug * plug)
{
	return plug->raw_value();
}
