#include <boost/uuid/nil_generator.hpp>
#include <memory>
#include "transports/ZstStageTransport.h"
#include "ZstStageMessage.h"

#include "adaptors/ZstTransportAdaptor.hpp"
#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstPlug.h"
#include "entities/ZstEntityFactory.h"

namespace showtime {

ZstStageMessage::ZstStageMessage() :
	m_origin_endpoint_UUID(nil_generator()()),
    m_buffer(NULL),
    m_id(nil_generator()()),
    m_transport_endpoint_UUID(nil_generator()())
{
}
    
ZstStageMessage::~ZstStageMessage(){
}
 
void ZstStageMessage::init(const StageMessage * buffer, uuid& origin_uuid, uuid& msg_id, std::shared_ptr<ZstStageTransport>& owning_transport)
{
	reset();
	m_buffer = buffer;

    m_id = msg_id;
    m_origin_endpoint_UUID = origin_uuid;
    m_owning_transport = owning_transport;
}

void ZstStageMessage::reset() {
	m_origin_endpoint_UUID = nil_generator()();
	m_buffer = NULL;
    m_owning_transport.reset();
}

const uuid& ZstStageMessage::origin_endpoint_UUID() const
{
	return m_origin_endpoint_UUID;
}

ZstMsgID ZstStageMessage::id() const
{
	return m_id;
}
    
Content ZstStageMessage::type() const {
    return m_buffer->content_type();
}
    
const StageMessage* ZstStageMessage::buffer() const {
    return m_buffer;
}

std::shared_ptr<ZstTransportLayerBase> ZstStageMessage::owning_transport() const
{
    auto transport = m_owning_transport.lock();
    auto cast = std::static_pointer_cast<ZstTransportLayerBase>(transport);
    return cast;
}

}
