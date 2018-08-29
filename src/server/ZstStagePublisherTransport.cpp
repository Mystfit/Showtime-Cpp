#include "ZstStagePublisherTransport.h"

ZstStagePublisherTransport::ZstStagePublisherTransport()
{
}

ZstStagePublisherTransport::~ZstStagePublisherTransport()
{
}

void ZstStagePublisherTransport::init()
{
	ZstTransportLayerBase::init();

	std::stringstream addr;
	addr << "@tcp://*:" << STAGE_PUB_PORT;
	m_graph_update_pub = zsock_new_pub(addr.str().c_str());
	if (!m_graph_update_pub) {
		ZstLog::net(LogLevel::notification, "Could not bind stage graph publisher socket to {}", addr.str());
		return;
	}
	zsock_set_linger(m_graph_update_pub, 0);

	ZstLog::net(LogLevel::notification, "Stage publisher sending via address {}", addr.str());
}

void ZstStagePublisherTransport::destroy()
{
	ZstTransportLayerBase::destroy();
	if(m_graph_update_pub)
		zsock_destroy(&m_graph_update_pub);
}

void ZstStagePublisherTransport::send_message_impl(ZstMessage * msg)
{
	zmsg_t * msg_handle = msg->handle();
	zmsg_send(&msg_handle, m_graph_update_pub);
	release_msg(static_cast<ZstStageMessage*>(msg));
}

void ZstStagePublisherTransport::on_receive_msg(ZstMessage * msg)
{
	ZstTransportLayerBase::on_receive_msg(msg);

	//Publish message to other modules
	msg_events()->invoke([msg](ZstTransportAdaptor * adaptor) {
		adaptor->on_receive_msg(msg);
	});
	release_msg(static_cast<ZstStageMessage*>(msg));
}
