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

void ZstPlugLiason::output_plug_set_fire_control_owner(ZstOutputPlug* plug, const ZstURI& owner)
{
	if (!plug) return;
	plug->set_fire_control_owner(owner);
}

void ZstPlugLiason::output_plug_set_transport(ZstOutputPlug * plug, ZstTransportAdaptor * transport)
{
	if (!plug || !transport) return;
	plug->add_adaptor(transport);
}
