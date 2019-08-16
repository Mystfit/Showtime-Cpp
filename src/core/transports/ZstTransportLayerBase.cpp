#include "ZstTransportLayerBase.hpp"
#include "../adaptors/ZstTransportAdaptor.hpp"
#include "ZstConstants.h"

ZstTransportLayerBase::ZstTransportLayerBase() :
	ZstMessageSupervisor(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT),
	m_is_active(false),
    m_dispatch_events(NULL)
{
	m_dispatch_events = new ZstEventDispatcher<ZstTransportAdaptor*>("transport events");
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

void ZstTransportLayerBase::init()
{
	m_is_active = true;
}

ZstMessageReceipt ZstTransportLayerBase::send_sync_message(ZstMessage* msg, const ZstTransportArgs& args)
{
	auto future = register_response(msg->id());
	ZstMessageReceipt msg_response{ ZstMsgKind::EMPTY, ZstTransportRequestBehaviour::SYNC_REPLY };

	send_message_impl(msg, args);
	try {
		msg_response.status = future.get();
	}
	catch (const ZstTimeoutException& e) {
		ZstLog::net(LogLevel::error, "Server response timed out", e.what());
		msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
	}

	enqueue_resolved_promise(msg->id());
	return msg_response;
}

void ZstTransportLayerBase::send_async_message(ZstMessage* msg, const ZstTransportArgs& args)
{
	auto future = register_response(msg->id());

	//Hold on to the message id so we can clean up the promise in case we time out
	ZstMsgID id = msg->id();

	//Copy receive action so the lambda can reference it when the response arrives
	auto completed_action = args.msg_receive_action;

	future.then([this, id, completed_action](ZstMessageFuture f) {
		ZstMsgKind status(ZstMsgKind::EMPTY);
		ZstMessageReceipt msg_response{ status, ZstTransportRequestBehaviour::ASYNC_REPLY };
		try {
			ZstMsgKind status = f.get();
			msg_response.status = status;
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
	send_message_impl(msg, ZstTransportArgs());
}

void ZstTransportLayerBase::begin_send_message(ZstMessage * msg, const ZstTransportArgs& args)
{
	if (!msg) return;

	switch (args.msg_send_behaviour) {
	case ZstTransportRequestBehaviour::ASYNC_REPLY:
		send_async_message(msg, args);
		break;
	case ZstTransportRequestBehaviour::SYNC_REPLY:
		args.msg_receive_action(send_sync_message(msg, args));
		break;
	case ZstTransportRequestBehaviour::PUBLISH:
		send_message_impl(msg, args);
		break;
	default:
		ZstTransportLayerBase::begin_send_message(msg, args);
		break;
	}
}

void ZstTransportLayerBase::receive_msg(ZstMessage * msg)
{
	msg_events()->defer([msg](ZstTransportAdaptor* adaptor) {
		adaptor->on_receive_msg(msg);
	}, [this, msg](ZstEventStatus status) {
		process_response(msg->id(), msg->kind());
	});
}

void ZstTransportLayerBase::receive_msg(ZstMessage* msg, ZstEventCallback on_complete)
{
	msg_events()->defer([msg](ZstTransportAdaptor* adaptor) { 
		adaptor->on_receive_msg(msg); 
	}, [this, msg, on_complete](ZstEventStatus status) {
		process_response(msg->id(), msg->kind());
		on_complete(status);
	});
}
