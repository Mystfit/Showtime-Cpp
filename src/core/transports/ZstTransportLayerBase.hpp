#pragma once

#include <string>
#include <functional>
#include <ZstExports.h>
#include "../ZstEventDispatcher.hpp"
#include "../ZstActor.h"
#include "../ZstMessage.h"

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


class ZstTransportLayerBase {
public:
	ZST_EXPORT ZstTransportLayerBase();
	ZST_EXPORT virtual ~ZstTransportLayerBase();

	ZST_EXPORT virtual void init(std::shared_ptr<ZstActor> reactor);
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void process_events();

	ZST_EXPORT virtual void send_message_impl(ZstMessage * msg) = 0;
	ZST_EXPORT void begin_send_message(ZstMessage * msg);
	ZST_EXPORT virtual void begin_send_message(ZstMessage * msg, const ZstTransportSendType & sendtype, const MessageReceivedAction & action);
	ZST_EXPORT virtual void on_receive_msg(ZstMessage * msg);

	ZST_EXPORT ZstEventDispatcher<ZstTransportAdaptor*> * msg_events();
	ZST_EXPORT bool is_active();

protected:
	ZST_EXPORT std::shared_ptr<ZstActor> get_reactor();

private:
	bool m_is_active;
	std::shared_ptr<ZstActor> m_reactor;
	ZstEventDispatcher<ZstTransportAdaptor*> * m_dispatch_events;
};
