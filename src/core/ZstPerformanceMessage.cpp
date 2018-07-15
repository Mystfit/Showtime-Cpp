#include "ZstPerformanceMessage.h"

ZstPerformanceMessage::~ZstPerformanceMessage()
{
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind)
{
	ZstMessage::init();
	ZstMessage::init(kind);
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	ZstMessage::init();
	ZstMessage::init(kind, args);
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	ZstMessage::init();
	ZstMessage::init(kind, serialisable);
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args)
{
	ZstMessage::init();
	ZstMessage::init(kind, serialisable, args);
	return this;
}
