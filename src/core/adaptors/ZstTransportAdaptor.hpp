#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>
#include "../ZstTransportLayerBase.hpp"

//Forwards
class ZstMessage;

class ZstTransportAdaptor : public ZstEventAdaptor
{
public:
	ZST_EXPORT virtual void on_receive_msg(ZstMessage * msg);

	ZST_EXPORT virtual void send_message(ZstMsgKind kind);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstMsgArgs & args);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstSerialisable & serialisable);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstMsgArgs & args, const ZstSerialisable & serialisable);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const MessageReceivedAction & action);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & args, const MessageReceivedAction & action);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const MessageReceivedAction & action);
	ZST_EXPORT virtual void send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const ZstMsgArgs & args, const MessageReceivedAction & action);
};