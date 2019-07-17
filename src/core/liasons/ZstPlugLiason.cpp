#include "ZstPlugLiason.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"
#include "../ZstEventDispatcher.hpp"

void ZstPlugLiason::plug_remove_cable(ZstPlug * plug, ZstCable * cable)
{
	if (!plug || !cable) return;
	plug->remove_cable(cable);
}

void ZstPlugLiason::plug_add_cable(ZstPlug * plug, ZstCable * cable)
{
	if (!plug || !cable) return;
	plug->add_cable(cable);
}

void ZstPlugLiason::output_plug_set_can_fire(ZstOutputPlug* plug, bool can_fire)
{
	plug->set_can_fire(can_fire);
}

void ZstPlugLiason::output_plug_set_transport(ZstOutputPlug * plug, ZstTransportAdaptor * transport)
{
	if (!plug || !transport) return;
	plug->add_adaptor(transport);
}
