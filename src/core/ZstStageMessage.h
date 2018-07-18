#pragma once

#include "ZstMessage.h"

class ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other);
    ZST_EXPORT virtual ~ZstStageMessage();

    ZST_EXPORT void reset() override;

	ZST_EXPORT virtual ZstStageMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstStageMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstStageMessage * init(ZstMsgKind kind, const ZstSerialisable & serialisable) override;
	ZST_EXPORT virtual ZstStageMessage * init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args) override;
	
    ZST_EXPORT void unpack(zmsg_t * msg) override;
};
