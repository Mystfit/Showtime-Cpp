#pragma once

#include "ZstExports.h"
#include "ZstMsgID.h"
#include <memory>
#include <boost/uuid/uuid.hpp>

using namespace boost::uuids;

namespace showtime {

    //Forwards
    class ZstTransportLayerBase;

class ZST_CLASS_EXPORTED ZstMessage {
public:
    ZST_EXPORT ZstMessage();
    ZST_EXPORT ZstMessage(const ZstMessage& other) = delete;

    ZST_EXPORT virtual void reset() = 0;
    ZST_EXPORT virtual ZstMsgID id() const = 0;
    ZST_EXPORT virtual const uuid& origin_endpoint_UUID() const = 0;
    ZST_EXPORT virtual std::shared_ptr<ZstTransportLayerBase> owning_transport() const = 0;

    ZST_EXPORT void set_has_promise();
    ZST_EXPORT bool has_promise();

private:
    bool m_response;
};

}
