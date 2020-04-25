#include "ZstPerformanceMessage.h"
#include "transports/ZstGraphTransport.h"
#include <boost/uuid/nil_generator.hpp>
#include <exception>

namespace showtime {

ZstPerformanceMessage::ZstPerformanceMessage() : 
    m_origin_endpoint_UUID(nil_generator()()),
    m_buffer(NULL)
{
}

ZstPerformanceMessage::~ZstPerformanceMessage()
{    
}

void ZstPerformanceMessage::init(const GraphMessage * buffer, std::shared_ptr<ZstGraphTransport>& owning_transport)
{
	reset();
    m_buffer = buffer;
    m_owning_transport = owning_transport;
}

void ZstPerformanceMessage::reset()
{
    //if(m_buffer)
    //    delete m_buffer;
    //
    m_buffer = NULL;
    m_sender = "";
    m_origin_endpoint_UUID = nil_generator()();
    m_owning_transport.reset();
}

ZstMsgID ZstPerformanceMessage::id() const
{
	return nil_generator()();
}
    
const uuid& ZstPerformanceMessage::origin_endpoint_UUID() const
{
	return m_origin_endpoint_UUID;
}

const GraphMessage* ZstPerformanceMessage::buffer() const
{
    return m_buffer;
}

std::shared_ptr<ZstTransportLayerBase> ZstPerformanceMessage::owning_transport() const
{
    return std::static_pointer_cast<ZstTransportLayerBase>(m_owning_transport.lock());

}
    
}
