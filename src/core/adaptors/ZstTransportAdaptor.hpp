#pragma once

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"
#include "../transports/ZstTransportLayerBase.hpp"

//Forwards
class ZstMessage;

class ZstTransportAdaptor : public ZstEventAdaptor
{
public:
	//Outgoing Events
	ZST_EXPORT virtual void on_receive_msg(ZstMessage * msg);

	//Incoming events
	ZST_EXPORT virtual void connect(const std::string & address);
	ZST_EXPORT virtual void disconnect();
	ZST_EXPORT virtual void bind(const std::string& address);
	ZST_EXPORT virtual void send_msg(ZstMsgKind kind);
	ZST_EXPORT virtual void send_msg(ZstMsgKind kind, const ZstMsgArgs & args);
	ZST_EXPORT virtual void send_msg(ZstMsgKind kind, const ZstMsgArgs & args, const ZstMsgArgs & payload);
	ZST_EXPORT virtual void send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const MessageReceivedAction & action);
	ZST_EXPORT virtual void send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & args, const MessageReceivedAction & action);
	ZST_EXPORT virtual void send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & payload, const ZstMsgArgs & args, const MessageReceivedAction & action);
};
