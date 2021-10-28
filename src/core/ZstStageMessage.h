#pragma once

#include <showtime/schemas/messaging/stage_message_generated.h>
#include <showtime/ZstExports.h>
#include <showtime/entities/ZstEntityBase.h>

#include "ZstMessage.h"


namespace showtime {
	
// Forwards
class ZstStageTransport;

class ZST_CLASS_EXPORTED ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other) = delete;
    ZST_EXPORT virtual ~ZstStageMessage();
	
	ZST_EXPORT void init(const StageMessage * buffer, uuid& origin_uuid, uuid& msg_id, std::shared_ptr<ZstStageTransport>& owning_transport);
	ZST_EXPORT virtual void reset() override;
	
	ZST_EXPORT virtual ZstMsgID id() const override;
    ZST_EXPORT Content type() const;
    ZST_EXPORT const StageMessage* buffer() const;
	ZST_EXPORT const uuid& origin_endpoint_UUID() const override;
	ZST_EXPORT std::shared_ptr<ZstTransportLayerBase> owning_transport() const override;

private:
	boost::uuids::uuid m_transport_endpoint_UUID;
	boost::uuids::uuid m_origin_endpoint_UUID;
	boost::uuids::uuid m_id;
	const StageMessage * m_buffer;
	std::weak_ptr<ZstStageTransport> m_owning_transport;
};

}
