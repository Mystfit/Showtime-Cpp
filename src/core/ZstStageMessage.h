#pragma once

#include <schemas/stage_message_generated.h>

#include "ZstExports.h"
#include "ZstMessage.h"
#include "entities/ZstEntityBase.h"
#include "transports/ZstTransportLayerBase.hpp"

namespace showtime {

class ZST_CLASS_EXPORTED ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other) = delete;
	
	ZST_EXPORT void init(StageMessage * buffer);
	ZST_EXPORT void reset() override;
	
	ZST_EXPORT virtual ZstMsgID id() const override;
	ZST_EXPORT Content type();
	
	template<typename T>
	ZST_EXPORT const T * content(){
		if(!m_buffer){
			return NULL;
		}
		return m_buffer->content_as<T>();
	}

	ZST_EXPORT void set_endpoint_UUID(const uuid&);
	ZST_EXPORT const uuid& endpoint_UUID() const override;

private:
	boost::uuids::uuid m_endpoint_UUID;
	StageMessage * m_buffer;
};

}
