//
//  ZstGraphTransportAdaptor.hpp
//  Showtime
//
//  Created by Byron Mallett on 19/09/19.
//

#pragma once

#include "showtime/schemas/messaging/graph_message_generated.h"

#include <showtime/ZstExports.h>
#include "ZstTransportAdaptor.hpp"
#include "../ZstPerformanceMessage.h"
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {

    class ZST_CLASS_EXPORTED ZstGraphTransportAdaptor : public ZstTransportAdaptor
    {
    public:
        ZST_EXPORT ZstGraphTransportAdaptor();
        MULTICAST_DELEGATE_OneParam(ZST_EXPORT, receive_msg, const std::shared_ptr<ZstPerformanceMessage>&, msg)
        
        // ---
        ZST_EXPORT virtual flatbuffers::DetachedBuffer create_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder) {
            return flatbuffers::DetachedBuffer();
        }
        ZST_EXPORT virtual ZstMessageReceipt send_msg(flatbuffers::DetachedBuffer&& message_buffer, const ZstTransportArgs& args = {}) {
            return ZstMessageReceipt{ Signal_EMPTY };
        };
    };
}
