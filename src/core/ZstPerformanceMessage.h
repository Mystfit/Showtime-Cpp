#pragma once

#include "ZstMessage.h"
#include "liasons/ZstPlugLiason.hpp"

class ZstPerformanceMessage : public ZstMessage, public ZstPlugLiason {
public:
    ZST_EXPORT virtual ~ZstPerformanceMessage();
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstSerialisable & serialisable) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args) override;
};
