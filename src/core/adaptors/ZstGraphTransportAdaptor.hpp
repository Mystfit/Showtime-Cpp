//
//  ZstGraphTransportAdaptor.hpp
//  Showtime
//
//  Created by Byron Mallett on 19/09/19.
//

#pragma once

#include <schemas/graph_message_generated.h>

#include "ZstExports.h"
#include "ZstPerformanceMessage.h"
#include "adaptors/ZstEventAdaptor.hpp"
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {

    class ZST_CLASS_EXPORTED ZstGraphTransportAdaptor : public ZstTransportAdaptor
    {
    public:
        ZST_EXPORT virtual void on_receive_msg(ZstPerformanceMessage * msg);
        ZST_EXPORT virtual ZstMessageReceipt send_msg(GraphMessageBuilder & message, const ZstTransportArgs& args = {});
    };
}
