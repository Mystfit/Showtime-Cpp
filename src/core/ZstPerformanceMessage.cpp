#include "ZstPerformanceMessage.h"

ZstPerformanceMessage::~ZstPerformanceMessage()
{
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind)
{
	ZstMessage::init();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	zmsg_addmem(this->handle(), buffer.str().c_str(), buffer.str().size());
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	ZstMessage::init();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	this->append_args(args, buffer);
	zmsg_addmem(this->handle(), buffer.str().c_str(), buffer.str().size());
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	ZstMessage::init();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	this->append_args({}, buffer);
	this->append_payload(serialisable, buffer);
	zmsg_addmem(this->handle(), buffer.str().c_str(), buffer.str().size());
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args)
{
	ZstMessage::init();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	this->append_args(args, buffer);
	this->append_payload(serialisable, buffer);
	zmsg_addmem(this->handle(), buffer.str().c_str(), buffer.str().size());
	return this;
}
