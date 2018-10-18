#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>
#include "../transports/ZstTransportLayerBase.hpp"

//Forwards
class ZstMessage;

class ZstTransportAdaptor : public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_receive_msg(ZstMessage * msg);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const ZstMsgArgs & args);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const std::string & payload);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const ZstMsgArgs & args, const std::string & payload);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const MessageReceivedAction & action);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & args, const MessageReceivedAction & action);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const std::string & payload, const MessageReceivedAction & action);
	ZST_EXPORT virtual void on_send_msg(ZstMsgKind kind, const ZstTransportSendType & sendtype, const std::string & payload, const ZstMsgArgs & args, const MessageReceivedAction & action);
};