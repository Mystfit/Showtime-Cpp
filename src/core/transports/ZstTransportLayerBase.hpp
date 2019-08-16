#pragma once

#include <string>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "ZstExports.h"
#include "../ZstEventDispatcher.hpp"
#include "../ZstActor.h"
#include "../ZstMessage.h"
#include "../ZstMessageSupervisor.hpp"



//Forwards
class ZstTransportAdaptor;

enum ZstTransportRequestBehaviour
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
	ZstTransportRequestBehaviour sendtype;
};

typedef std::function<void(ZstMessageReceipt)> ZstMessageReceivedAction;

struct ZstTransportArgs {
	boost::uuids::uuid target_endpoint_UUID = boost::uuids::nil_generator()();
	ZstTransportRequestBehaviour msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	ZstMsgID msg_ID = 0;
	ZstMsgArgs msg_args;
	ZstMsgArgs msg_payload;
	ZstMessageReceivedAction msg_receive_action = [](ZstMessageReceipt receipt) {};
};


class ZstTransportLayerBase : public ZstMessageSupervisor {
public:
	ZST_EXPORT ZstTransportLayerBase();
	ZST_EXPORT virtual ~ZstTransportLayerBase();

	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void process_events();

	ZST_EXPORT virtual void begin_send_message(ZstMessage* msg);
	ZST_EXPORT virtual void begin_send_message(ZstMessage* msg, const ZstTransportArgs & args);
	ZST_EXPORT virtual void receive_msg(ZstMessage* msg);
	ZST_EXPORT virtual void receive_msg(ZstMessage* msg, ZstEventCallback on_complete);

	//Message sending implementation for the transport
	ZST_EXPORT virtual void send_message_impl(ZstMessage * msg, const ZstTransportArgs & args) = 0;

	//Message supervision
	ZST_EXPORT virtual ZstMessageReceipt send_sync_message(ZstMessage* msg, const ZstTransportArgs& args);
	ZST_EXPORT virtual void send_async_message(ZstMessage* msg, const ZstTransportArgs& args);

	ZST_EXPORT ZstEventDispatcher<ZstTransportAdaptor*> * msg_events();
	ZST_EXPORT bool is_active();


private:
	bool m_is_active;

	ZstEventDispatcher<ZstTransportAdaptor*> * m_dispatch_events;
};
