#include "ZstTransportLayerBase.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"
#include "ZstConstants.h"
#include "ZstExceptions.h"

namespace showtime {

ZstTransportLayerBase::ZstTransportLayerBase() :
	ZstMessageSupervisor(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT),
	m_is_active(false),
    m_dispatch_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > >("transport events"))
{
}

ZstTransportLayerBase::~ZstTransportLayerBase()
{
}

void ZstTransportLayerBase::destroy()
{
	
}

void ZstTransportLayerBase::init()
{
	m_is_active = true;
}

ZstMessageReceipt ZstTransportLayerBase::send_sync_message(ZstMessage* msg, const ZstTransportArgs& args)
{
	auto future = register_response(msg->id());
	ZstMessageReceipt msg_response{ ZstMsgKind::EMPTY };

	send_message_impl(msg, args);
	try {
		//Blocking call on get
		msg_response = future.get();
	}
	catch (const ZstTimeoutException& e) {
		ZstLog::net(LogLevel::error, "Server response timed out", e.what());
		msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
	}

	enqueue_resolved_promise(msg->id());
	msg_response.request_behaviour = ZstTransportRequestBehaviour::SYNC_REPLY;
	return msg_response;
}

ZstMessageReceipt ZstTransportLayerBase::send_async_message(ZstMessage* msg, const ZstTransportArgs& args)
{
	auto future = register_response(msg->id());

	//Hold on to the message id so we can clean up the promise in case we time out
	ZstMsgID id = msg->id();

	//Copy receive action so the lambda can reference it when the response arrives
	auto completed_action = args.on_recv_response;

	future.then([this, id, completed_action](ZstMessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		ZstMessageReceipt msg_response{ status };
		try {
			ZstMsgKind status = f.get().status;
			msg_response.status = status;
			msg_response.request_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
			completed_action(msg_response);
			return status;
		}
		catch (const ZstTimeoutException& e) {
			ZstLog::net(LogLevel::error, "Server async response timed out - {}", e.what());
			msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
			completed_action(msg_response);
			enqueue_resolved_promise(id);
		}
		return status;
		});

	send_message_impl(msg, args);
	return ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportRequestBehaviour::ASYNC_REPLY };
}

std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > > & ZstTransportLayerBase::msg_events()
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

ZstMessageReceipt ZstTransportLayerBase::begin_send_message(ZstMessage * msg)
{
	send_message_impl(msg, ZstTransportArgs());
	return ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportRequestBehaviour::PUBLISH };
}

ZstMessageReceipt ZstTransportLayerBase::begin_send_message(ZstMessage * msg, const ZstTransportArgs& args)
{
	switch (args.msg_send_behaviour) {
	case ZstTransportRequestBehaviour::ASYNC_REPLY:
		return send_async_message(msg, args);
		break;
	case ZstTransportRequestBehaviour::SYNC_REPLY:
	{
		auto receipt = send_sync_message(msg, args);
		args.on_recv_response(receipt);
		return receipt;
		break;
	}
	case ZstTransportRequestBehaviour::PUBLISH: {
		send_message_impl(msg, args);
		return ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportRequestBehaviour::PUBLISH };
		break;
	}
	default:
		ZstLog::net(LogLevel::error, "Can't send message. Unknown message request behaviour");
		break;
	}
	return ZstMessageReceipt{ ZstMsgKind::EMPTY };
}

void ZstTransportLayerBase::receive_msg(ZstMessage * msg)
{
	msg_events()->defer([msg](std::shared_ptr<ZstTransportAdaptor> adaptor) {
		adaptor->on_receive_msg(msg);
	}, [this, msg](ZstEventStatus status) {
		process_response(msg->id(), ZstMessageReceipt{ msg->kind() });
	});
}

void ZstTransportLayerBase::receive_msg(ZstMessage* msg, ZstEventCallback on_complete)
{
	msg_events()->defer([msg](std::shared_ptr<ZstTransportAdaptor> adaptor) {
		adaptor->on_receive_msg(msg);
	}, [this, msg, on_complete](ZstEventStatus status) {
		process_response(msg->id(), ZstMessageReceipt{ msg->kind() });
		on_complete(status);
	});
}

}
