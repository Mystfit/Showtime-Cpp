#include "ZstPerformanceMessage.h"

ZstPerformanceMessage::~ZstPerformanceMessage()
{
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind)
{
	//Reset instead of init to skip creating a new zmsg_t* object
	ZstMessage::reset();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	m_payload = buffer.str();
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	//Reset instead of init to skip creating a new zmsg_t* object
	ZstMessage::reset();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	this->append_args(args, buffer);
	m_payload = buffer.str();
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const std::string & payload)
{
	//Reset instead of init to skip creating a new zmsg_t* object
	ZstMessage::reset();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	this->append_args({}, buffer);
	this->append_payload(payload, buffer);
	m_payload = buffer.str();
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const std::string & payload, const ZstMsgArgs & args)
{
	//Reset instead of init to skip creating a new zmsg_t* object
	ZstMessage::reset();
	std::stringstream buffer;
	this->append_kind(kind, buffer);
	this->append_args(args, buffer);
	this->append_payload(payload, buffer);
	m_payload = buffer.str();
	return this;
}
