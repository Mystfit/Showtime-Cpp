//
//  ZstStageTransportAdaptor.hpp
//  Showtime
//
//  Created by Byron Mallett on 19/09/19.
//
#pragma once

#include "showtime/schemas/messaging/stage_message_generated.h"

#include <showtime/ZstExports.h>
#include "ZstTransportAdaptor.hpp"
#include "../ZstStageMessage.h"
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {
    
    class ZST_CLASS_EXPORTED ZstStageTransportAdaptor : public ZstTransportAdaptor
    {
    public:
        ZST_EXPORT ZstStageTransportAdaptor();
        MULTICAST_DELEGATE_OneParam(ZST_EXPORT, receive_msg, const std::shared_ptr<ZstStageMessage>&, msg)

        // ----
        ZST_EXPORT virtual flatbuffers::DetachedBuffer create_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder) {
            return flatbuffers::DetachedBuffer();
        };
        ZST_EXPORT virtual ZstMessageReceipt send_msg(flatbuffers::DetachedBuffer&& message_buffer, const ZstTransportArgs& args = {}) {
            return ZstMessageReceipt(Signal_EMPTY);
        };
    };
}
