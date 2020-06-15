//
//  ZstGraphTransportAdaptor.hpp
//  Showtime
//
//  Created by Byron Mallett on 19/09/19.
//

#pragma once

#include "showtime/schemas/messaging/graph_message_generated.h"

#include "ZstExports.h"
#include "ZstTransportAdaptor.hpp"
#include "../ZstPerformanceMessage.h"
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {

    class ZST_CLASS_EXPORTED ZstGraphTransportAdaptor : public ZstTransportAdaptor
    {
    public:
        ZST_EXPORT virtual void on_receive_msg(const std::shared_ptr<ZstPerformanceMessage>& msg);
        ZST_EXPORT virtual ZstMessageReceipt send_msg(flatbuffers::Offset<GraphMessage> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args = {});
    };
}
