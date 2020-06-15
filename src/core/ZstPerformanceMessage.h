#pragma once

#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include "showtime/schemas/messaging/graph_message_generated.h"
#include "ZstMessage.h"

namespace showtime {

    class ZstGraphTransport;

class ZST_CLASS_EXPORTED ZstPerformanceMessage : public ZstMessage {
public:
    ZST_EXPORT ZstPerformanceMessage();
    ZST_EXPORT ZstPerformanceMessage(const ZstPerformanceMessage & other) = delete;
    ZST_EXPORT virtual ~ZstPerformanceMessage();
    
    ZST_EXPORT void init(const GraphMessage * buffer, std::shared_ptr<ZstGraphTransport>& owning_transport);
    ZST_EXPORT void reset() override;
    
    ZST_EXPORT virtual ZstMsgID id() const override;
    
    ZST_EXPORT void set_origin_endpoint_UUID(const uuid&);
    ZST_EXPORT const uuid& origin_endpoint_UUID() const override;
    ZST_EXPORT const GraphMessage* buffer() const;
    ZST_EXPORT virtual std::shared_ptr<ZstTransportLayerBase> owning_transport() const override;

private:
    std::string m_sender;
	boost::uuids::uuid m_origin_endpoint_UUID;
    const GraphMessage * m_buffer;
    std::weak_ptr<ZstGraphTransport> m_owning_transport;
};

}
