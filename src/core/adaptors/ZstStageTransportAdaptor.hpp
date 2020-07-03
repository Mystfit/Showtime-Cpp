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
        MULTICAST_DELEGATE_OneParam(ZST_EXPORT, receive_msg, const std::shared_ptr<ZstStageMessage>&, msg)

        // ----

        ZST_EXPORT virtual ZstMessageReceipt send_msg(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args = {});
    };
}
