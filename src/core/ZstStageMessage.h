#pragma once

#include <schemas/stage_message_generated.h>

#include "ZstExports.h"
#include "ZstMessage.h"
#include "entities/ZstEntityBase.h"

namespace showtime {

class ZST_CLASS_EXPORTED ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other) = delete;
    ZST_EXPORT virtual ~ZstStageMessage();
	
	ZST_EXPORT void init(const StageMessage * buffer);
	ZST_EXPORT void reset();
	
	ZST_EXPORT virtual ZstMsgID id() const;
    ZST_EXPORT Content type() const;
    ZST_EXPORT const StageMessage* buffer() const;

	ZST_EXPORT void set_endpoint_UUID(const uuid&);
	ZST_EXPORT const uuid& endpoint_UUID() const;

private:
	boost::uuids::uuid m_endpoint_UUID;
	const StageMessage * m_buffer;
};

}
