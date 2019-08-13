#pragma once

#include "ZstExports.h"
#include "ZstMessageOptions.h"
#include "ZstMsgID.h"
#include <nlohmann/json.hpp>


class ZstMessage {
public:
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind) = 0;
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) = 0;
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args) = 0;

    ZST_EXPORT virtual void reset() = 0;

	ZST_EXPORT virtual const ZstMsgKind& kind() const = 0;
	ZST_EXPORT virtual ZstMsgID id() const = 0;
	ZST_EXPORT virtual void set_id(const ZstMsgID& id) = 0;
    ZST_EXPORT virtual const ZstMsgArgs & payload() const = 0;
};
