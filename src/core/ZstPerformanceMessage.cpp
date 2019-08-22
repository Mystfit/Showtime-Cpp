#include "ZstPerformanceMessage.h"
#include <msgpack.hpp>
#include <exception>

ZstPerformanceMessage::ZstPerformanceMessage() : m_endpoint_UUID(nil_generator()())
{
}

ZstPerformanceMessage::~ZstPerformanceMessage()
{
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind)
{
	throw new std::runtime_error("Performance message init(kind) not implemented");
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	reset();
	set_sender(args.at(get_msg_arg_name(ZstMsgArg::PATH)));
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args, const ZstMsgPayload& payload)
{
	reset();
	set_sender(args.at(get_msg_arg_name(ZstMsgArg::PATH)));
	set_payload(payload);
	return this;
}

void ZstPerformanceMessage::reset()
{
	m_args.clear();
}

ZstMsgKind ZstPerformanceMessage::kind() const
{
	return ZstMsgKind::PERFORMANCE_MSG;
}

ZstMsgID ZstPerformanceMessage::id() const
{
	return 0;
}

void ZstPerformanceMessage::set_id(const ZstMsgID& id)
{
}

void ZstPerformanceMessage::unpack(const char * data, const size_t & size)
{
	m_args = json::from_msgpack(data, size);
}

const ZstMsgArgs & ZstPerformanceMessage::payload() const
{
	auto j_it = m_args.find(get_msg_arg_name(ZstMsgArg::PAYLOAD_SHORT));
	if (j_it != m_args.end())
		return *j_it;
    return std::move(json());
}

std::string ZstPerformanceMessage::sender() const
{
	return m_args[get_msg_arg_name(ZstMsgArg::SENDER_SHORT)];
}

std::vector<uint8_t> ZstPerformanceMessage::to_msgpack() const
{
	return json::to_msgpack(m_args);
}

const uuid& ZstPerformanceMessage::endpoint_UUID() const
{
	return m_endpoint_UUID;
}

void ZstPerformanceMessage::set_payload(const ZstMsgArgs & payload)
{
	m_args[get_msg_arg_name(ZstMsgArg::PAYLOAD_SHORT)] = payload;
}

void ZstPerformanceMessage::set_sender(const std::string & sender)
{
	m_args[get_msg_arg_name(ZstMsgArg::SENDER_SHORT)] = sender;
}
