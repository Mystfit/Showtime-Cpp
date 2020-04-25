//
//  ZstStageTransportAdaptor.hpp
//  Showtime
//
//  Created by Byron Mallett on 19/09/19.
//
#pragma once

#include <schemas/stage_message_generated.h>

#include "ZstExports.h"
#include "ZstTransportAdaptor.hpp"
#include "../ZstStageMessage.h"
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {
    
    class ZST_CLASS_EXPORTED ZstStageTransportAdaptor : public ZstTransportAdaptor
    {
    public:
        ZST_EXPORT virtual void on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg);
        ZST_EXPORT virtual ZstMessageReceipt send_msg(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args = {});
    };
}
