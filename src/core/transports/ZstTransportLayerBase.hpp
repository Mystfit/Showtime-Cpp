#pragma once

#include <string>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "ZstExports.h"
#include "ZstTransportHelpers.h"

#include "../ZstEventDispatcher.hpp"
#include "../ZstActor.h"
#include "../ZstMessage.h"
#include "../ZstMessageSupervisor.hpp"

//Forwards
class ZstTransportAdaptor;

class ZST_CLASS_EXPORTED ZstTransportLayerBase : public ZstMessageSupervisor {
public:
	ZST_EXPORT ZstTransportLayerBase();
	ZST_EXPORT virtual ~ZstTransportLayerBase();

	ZST_EXPORT virtual void init();
	ZST_EXPORT virtual void destroy();
	ZST_EXPORT virtual void process_events();

	ZST_EXPORT virtual ZstMessageReceipt begin_send_message(ZstMessage* msg);
	ZST_EXPORT virtual ZstMessageReceipt begin_send_message(ZstMessage* msg, const ZstTransportArgs & args);
	ZST_EXPORT virtual void receive_msg(ZstMessage* msg);
	ZST_EXPORT virtual void receive_msg(ZstMessage* msg, ZstEventCallback on_complete);

	//Message sending implementation for the transport
	ZST_EXPORT virtual void send_message_impl(ZstMessage * msg, const ZstTransportArgs & args) = 0;

	//Message supervision
	ZST_EXPORT virtual ZstMessageReceipt send_sync_message(ZstMessage* msg, const ZstTransportArgs& args);
	ZST_EXPORT virtual ZstMessageReceipt send_async_message(ZstMessage* msg, const ZstTransportArgs& args);

	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > > & msg_events();
	ZST_EXPORT bool is_active();


private:
	bool m_is_active;

    std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstTransportAdaptor> > > m_dispatch_events;
};
