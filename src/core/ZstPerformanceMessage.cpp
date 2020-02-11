#include "ZstPerformanceMessage.h"
#include <boost/uuid/nil_generator.hpp>
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
    //if(m_buffer)
    //    delete m_buffer;
    //
    m_buffer = NULL;
    m_sender = "";
    m_endpoint_UUID = nil_generator()();
}

ZstMsgID ZstPerformanceMessage::id() const
{
	return nil_generator()();
}
    
const uuid& ZstPerformanceMessage::endpoint_UUID() const
{
	return m_endpoint_UUID;
}

const GraphMessage* ZstPerformanceMessage::buffer() const
{
    return m_buffer;
}
    
}
