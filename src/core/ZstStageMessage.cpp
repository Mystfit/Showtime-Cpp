#include <boost/uuid/nil_generator.hpp>

#include "ZstStageMessage.h"

#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstPlug.h"
#include "entities/ZstEntityFactory.h"

namespace showtime {

ZstStageMessage::ZstStageMessage() :
	m_endpoint_UUID(nil_generator()())
{
}
 
void ZstStageMessage::init(StageMessage * buffer)
{
	reset();
	m_buffer = buffer;
}

void ZstStageMessage::reset() {
	m_endpoint_UUID = nil_generator()();
    
    if(m_buffer)
        delete m_buffer;
	m_buffer = NULL;
}

void ZstStageMessage::set_endpoint_UUID(const uuid& uuid)
{
	m_endpoint_UUID = boost::uuids::uuid(uuid);
}

const uuid& ZstStageMessage::endpoint_UUID() const
{
	return m_endpoint_UUID;
}

ZstMsgID ZstStageMessage::id() const
{
	return m_buffer->id();
}

}
