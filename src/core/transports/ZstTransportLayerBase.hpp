#pragma once

#include <string>
#include <functional>
#include "ZstExports.h"
#include "../ZstEventDispatcher.hpp"
#include "../ZstActor.h"
#include "../ZstMessage.h"
#include "../ZstMessageSupervisor.hpp"


//Forwards
class ZstTransportAdaptor;

enum ZstTransportSendType
{
	SYNC_REPLY = 0,
	ASYNC_REPLY,
	PUBLISH
};

/**
* Struct:	ZstMessageReceipt
*
* Summary:	Message response from a message sent to the server.
*/
struct ZstMessageReceipt {
	ZstMsgKind status;
	ZstTransportSendType sendtype;
};

typedef std::function<void(ZstMessageReceipt)> MessageReceivedAction;


class ZST_EXPORT ZstTransportLayerBase : public ZstMessageSupervisor {
public:
	ZST_EXPORT ZstTransportLayerBase();
	ZST_EXPORT virtual ~ZstTransportLayerBase();

	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void process_events();

	ZST_EXPORT void begin_send_message(ZstMessage* msg);
	ZST_EXPORT virtual void begin_send_message(ZstMessage* msg, const ZstTransportSendType& sendtype, const MessageReceivedAction& action);
	ZST_EXPORT virtual void receive_msg(ZstMessage* msg);
	ZST_EXPORT virtual void receive_msg(ZstMessage* msg, ZstEventCallback on_complete);

	//Message sending implementation for the transport
	ZST_EXPORT virtual void send_message_impl(ZstMessage * msg) = 0;

	//Message supervision
	ZST_EXPORT virtual ZstMessageReceipt send_sync_message(ZstMessage* msg);
	ZST_EXPORT virtual void send_async_message(ZstMessage* msg, const MessageReceivedAction& completed_action);

	ZST_EXPORT ZstEventDispatcher<ZstTransportAdaptor*> * msg_events();
	ZST_EXPORT bool is_active();


private:
	bool m_is_active;

	ZstEventDispatcher<ZstTransportAdaptor*> * m_dispatch_events;
};
