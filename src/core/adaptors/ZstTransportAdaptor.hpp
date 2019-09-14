#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"
#include "../transports/ZstTransportLayerBase.hpp"
#include "../ZstMessageSupervisor.hpp"

namespace showtime {

//Forwards
class ZstMessage;

class ZST_CLASS_EXPORTED ZstTransportAdaptor : 
	public ZstEventAdaptor
{
public:
	//Outgoing Events
	ZST_EXPORT virtual void on_receive_msg(ZstMessage * msg);

	//Incoming events
	ZST_EXPORT virtual void connect(const std::string & address);
	ZST_EXPORT virtual void disconnect();
	ZST_EXPORT virtual void bind(const std::string& address);
	ZST_EXPORT virtual ZstMessageReceipt send_msg(ZstMsgKind kind);
	ZST_EXPORT virtual ZstMessageReceipt send_msg(ZstMsgKind kind, const ZstTransportArgs& args);
};

}
