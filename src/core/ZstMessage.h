#pragma once

#include "ZstExports.h"
#include "ZstMsgID.h"

#include <boost/uuid/uuid.hpp>

using namespace boost::uuids;

namespace showtime {

class ZST_CLASS_EXPORTED ZstMessage {
public:
    ZST_EXPORT virtual void reset() = 0;
    ZST_EXPORT virtual ZstMsgID id() const = 0;
    ZST_EXPORT virtual const uuid& endpoint_UUID() const = 0;
};
    
}
