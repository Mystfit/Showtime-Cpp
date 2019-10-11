#include "ZstPerformanceMessage.h"
#include <msgpack.hpp>
#include <exception>

namespace showtime {

ZstPerformanceMessage::ZstPerformanceMessage() : m_endpoint_UUID(nil_generator()())
{
}

ZstPerformanceMessage::~ZstPerformanceMessage()
{    
}

void ZstPerformanceMessage::init(const GraphMessage * buffer)
{
	reset();
    m_buffer = buffer;
}

void ZstPerformanceMessage::reset()
{
    if(m_buffer)
        delete m_buffer;
    
    m_buffer = NULL;
    m_sender = "";
    m_endpoint_UUID = nil_generator()();
}

ZstMsgID ZstPerformanceMessage::id() const
{
	return 0;
}
    
const uuid& ZstPerformanceMessage::endpoint_UUID() const
{
	return m_endpoint_UUID;
}

void ZstPerformanceMessage::set_sender(const std::string & sender)
{
    m_sender = sender;
}
    
const std::string & ZstPerformanceMessage::sender() const
{
    return m_sender;
}

const GraphMessage* ZstPerformanceMessage::buffer() const
{
    return m_buffer;
}
    
}
