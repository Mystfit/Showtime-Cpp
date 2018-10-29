#pragma once

#include <ZstExports.h>
#include "ZstMessageOptions.h"
#include "ZstMsgID.h"


class ZstMessage {
public:
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind) = 0;
    ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) = 0;
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args) = 0;

    ZST_EXPORT virtual void reset() = 0;

    ZST_EXPORT virtual const char * payload_data() const = 0;
	ZST_EXPORT virtual const size_t payload_size() const = 0;
};
