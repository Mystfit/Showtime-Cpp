#pragma once

#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <schemas/graph_message_generated.h>
#include "ZstMessage.h"

namespace showtime {

class ZST_CLASS_EXPORTED ZstPerformanceMessage : public ZstMessage {
public:
    ZST_EXPORT ZstPerformanceMessage();
    ZST_EXPORT ZstPerformanceMessage(const ZstPerformanceMessage & other) = delete;
    ZST_EXPORT virtual ~ZstPerformanceMessage();
    
    ZST_EXPORT void init(const GraphMessage * buffer);
    ZST_EXPORT void reset();
    
    ZST_EXPORT virtual ZstMsgID id() const;
    
    ZST_EXPORT void set_endpoint_UUID(const uuid&);
    ZST_EXPORT const uuid& endpoint_UUID() const;
    ZST_EXPORT const GraphMessage* buffer() const;

private:
    std::string m_sender;
	boost::uuids::uuid m_endpoint_UUID;
    const GraphMessage * m_buffer;
};

}
