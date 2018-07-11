#include "ZstPerformanceMessage.h"

ZST_EXPORT ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind)
{
	ZstMessage::init();
	ZstMessage::init(kind);
	return this;
}

ZST_EXPORT ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	ZstMessage::init();
	ZstMessage::init(kind, args);
	return this;
}

ZST_EXPORT ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	ZstMessage::init();
	ZstMessage::init(kind, serialisable);
	return this;
}

ZST_EXPORT ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args)
{
	ZstMessage::init();
	ZstMessage::init(kind, serialisable, args);
	return this;
}
