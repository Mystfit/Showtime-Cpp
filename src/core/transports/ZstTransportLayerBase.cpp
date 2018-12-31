#include "ZstTransportLayerBase.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"


ZstTransportLayerBase::ZstTransportLayerBase() :
	m_is_active(false),
    m_dispatch_events(NULL),
	m_reactor(NULL)
{
	m_dispatch_events = new ZstEventDispatcher<ZstTransportAdaptor*>("msgdispatch stage events");
}

ZstTransportLayerBase::~ZstTransportLayerBase()
{
	delete m_dispatch_events;
}

void ZstTransportLayerBase::destroy()
{
	m_is_active = false;
	m_dispatch_events->flush();
	m_dispatch_events->remove_all_adaptors();
}

void ZstTransportLayerBase::init(std::shared_ptr<ZstActor> reactor)
{
	m_is_active = true;
	m_reactor = reactor;
}

ZstEventDispatcher<ZstTransportAdaptor*>* ZstTransportLayerBase::msg_events()
{
	return m_dispatch_events;
}

bool ZstTransportLayerBase::is_active()
{
	return m_is_active;
}

void ZstTransportLayerBase::process_events()
{
	m_dispatch_events->process_events();
}

void ZstTransportLayerBase::begin_send_message(ZstMessage * msg)
{
	if (!msg) return;
	send_message_impl(msg);
}

void ZstTransportLayerBase::begin_send_message(ZstMessage * msg, const ZstTransportSendType & sendtype, const MessageReceivedAction & action)
{
	if (!msg) return;
	send_message_impl(msg);
}

void ZstTransportLayerBase::on_receive_msg(ZstMessage * msg)
{
}

std::shared_ptr<ZstActor> ZstTransportLayerBase::get_reactor() {
	return m_reactor;
}
