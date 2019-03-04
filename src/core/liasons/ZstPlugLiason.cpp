#include "ZstPlugLiason.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"
#include "../ZstEventDispatcher.hpp"

void ZstPlugLiason::plug_remove_cable(ZstPlug * plug, ZstCable * cable)
{
	plug->remove_cable(cable);
}

void ZstPlugLiason::plug_add_cable(ZstPlug * plug, ZstCable * cable)
{
	plug->add_cable(cable);
}

void ZstPlugLiason::output_plug_set_transport(ZstOutputPlug * plug, ZstTransportAdaptor * transport)
{
	plug->m_performance_events->add_adaptor(transport);
}
