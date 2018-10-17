#pragma once

#include <string>
#include <functional>
#include <cf/cfuture.h>
#include <cf/time_watcher.h>
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

typedef cf::promise<ZstMsgKind> MessagePromise;
typedef cf::future<ZstMsgKind> MessageFuture;
typedef std::function<void(ZstMessageReceipt)> MessageReceivedAction;


class ZstTransportLayerBase {
public:
	ZST_EXPORT ZstTransportLayerBase();
	ZST_EXPORT virtual ~ZstTransportLayerBase();

	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void process_events();

	ZST_EXPORT virtual void send_message_impl(ZstMessage * msg) = 0;
	ZST_EXPORT void begin_send_message(ZstMessage * msg);
	ZST_EXPORT void begin_send_message(ZstMessage * msg, const ZstTransportSendType & sendtype, const MessageReceivedAction & action);
	ZST_EXPORT virtual void on_receive_msg(ZstMessage * msg);

	ZST_EXPORT virtual void send_sock_msg(zsock_t * sock, ZstMessage * msg);
	ZST_EXPORT zmsg_t * sock_recv(zsock_t* socket, bool pop_first);

	ZST_EXPORT ZstEventDispatcher<ZstTransportAdaptor*> * msg_events();

	ZST_EXPORT bool is_active();

protected:
	ZST_EXPORT virtual ZstMessageReceipt send_sync_message(ZstMessage * msg);
	ZST_EXPORT virtual void send_async_message(ZstMessage * msg, const MessageReceivedAction & completed_action);

private:
	bool m_is_active;

	//Dispatcher methods

	MessageFuture register_response_message(ZstMessage * msg);
	void cleanup_response_message(ZstMsgID id);
	void process_responses(ZstMessage * msg);
	void copy_id_arg(ZstMessage * msg);

	std::unordered_map<ZstMsgID, MessagePromise > m_response_promises;
	moodycamel::ConcurrentQueue<ZstMsgID> m_dead_promises;
	cf::time_watcher * m_timeout_watcher;

	ZstEventDispatcher<ZstTransportAdaptor*> * m_dispatch_events;
};
