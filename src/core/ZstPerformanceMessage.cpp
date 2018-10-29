#include "ZstPerformanceMessage.h"
#include <msgpack.hpp>
#include <exception>

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind)
{
	throw new std::runtime_error("Performance message init(kind) not implemented");
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	reset();

	std::stringstream buffer;
	set_sender(args.at(get_msg_arg_name(ZstMsgArg::PATH)), buffer);

	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args)
{
	reset();
	
	std::stringstream buffer;
	set_sender(args.at(get_msg_arg_name(ZstMsgArg::PATH)), buffer);
	set_payload(payload, buffer);

	return this;
}

void ZstPerformanceMessage::reset()
{
	m_sender.clear();
	m_payload.clear();
}

void ZstPerformanceMessage::unpack(const char * data, const size_t & size)
{
	size_t offset = 0;

	auto handle = msgpack::unpack(data, size, offset);
	m_sender = std::move(std::string(handle.get().via.str.ptr, handle.get().via.str.size));

	//If we still have data at the end after unpacking the args, then this performance message has a payload
	if (offset < size) {
		handle = msgpack::unpack(data, size, offset);
		m_payload = std::move(std::string(handle.get().via.str.ptr, handle.get().via.str.size));
	}
}

const char * ZstPerformanceMessage::payload_data() const
{
	return m_payload.c_str();
}

const size_t ZstPerformanceMessage::payload_size() const
{
	return m_payload.size();
}

const std::string & ZstPerformanceMessage::sender() const
{
	return m_sender;
}

void ZstPerformanceMessage::set_payload(const std::string & payload, std::stringstream & buffer)
{
	m_payload = payload;
}

void ZstPerformanceMessage::set_sender(const std::string & sender, std::stringstream & buffer)
{
	m_sender = sender;
}
