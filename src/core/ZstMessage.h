#pragma once

#include "ZstExports.h"
#include "ZstMessageOptions.h"
#include "ZstMsgID.h"
#include <nlohmann/json.hpp>
#include <boost/uuid/uuid.hpp>

using namespace boost::uuids;

namespace showtime {

class ZST_CLASS_EXPORTED ZstMessage {
public:
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind) = 0;
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs& args) = 0;
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs& args, const ZstMsgPayload& payload) = 0;

    ZST_EXPORT virtual void reset() = 0;

    ZST_EXPORT virtual ZstMsgKind kind() const = 0;
    ZST_EXPORT virtual ZstMsgID id() const = 0;
    ZST_EXPORT virtual const uuid& endpoint_UUID() const = 0;
    ZST_EXPORT virtual const ZstMsgArgs & payload() const = 0;
};
    
}
