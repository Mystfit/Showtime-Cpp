#include "ZstStagePublisherTransport.h"
#include <czmq.h>

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
	zmsg_t * m = zmsg_new();

	ZstStageMessage * stage_msg = static_cast<ZstStageMessage*>(msg);
	zmsg_addstr(m, stage_msg->as_json_str().c_str());
	zmsg_send(&m, m_graph_update_pub);

	release_msg(stage_msg);
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
