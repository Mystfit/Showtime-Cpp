#pragma once

#include "ZstMessage.h"

class ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other);
    ZST_EXPORT ~ZstStageMessage();

    ZST_EXPORT void reset() override;

    ZST_EXPORT ZstStageMessage * init_entity_message(const ZstEntityBase * entity);
	ZST_EXPORT ZstStageMessage * init_message(ZstMsgKind kind);
	ZST_EXPORT ZstStageMessage * init_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable);

	ZST_EXPORT void copy_id(const ZstStageMessage * msg);

	ZST_EXPORT void append_entity_kind_frame(const ZstEntityBase * entity);
	ZST_EXPORT void append_kind_frame(ZstMsgKind k);
	ZST_EXPORT void append_id_frame();
	ZST_EXPORT void append_serialisable(ZstMsgKind k, const ZstSerialisable & s);

    ZST_EXPORT void unpack(zmsg_t * msg) override;
    ZstMsgKind unpack_kind();
	ZstMsgKind unpack_kind(zframe_t * kind_frame);

    //Accessors
    ZST_EXPORT const char * id() const;
	ZST_EXPORT const ZstMsgKind kind() const;

private:
    /** Summary:	The message kind. */
	ZstMsgKind m_msg_kind;

	/** Summary:	The message id. */
	char m_msg_id[ZSTMSG_UUID_LENGTH];
};
