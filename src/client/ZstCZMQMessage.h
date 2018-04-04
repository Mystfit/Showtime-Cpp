#pragma once

#include <czmq.h>
#include "../core/ZstMessage.h"

class ZstCZMQMessagePayload : public ZstMessagePayload {
public:
	ZstCZMQMessagePayload(ZstMsgKind k, void * p);
	ZstCZMQMessagePayload(const ZstCZMQMessagePayload & other);
	~ZstCZMQMessagePayload();
	size_t size() override;
	char * data() override;
};

class ZstCZMQMessage : public ZstMessage {
	friend class ZstCZMQMessagePool;

public:
	~ZstCZMQMessage();
	ZstCZMQMessage(const ZstCZMQMessage & other);
	void reset() override;
	void copy_id(const ZstCZMQMessage * msg);

	ZstCZMQMessage * init_entity_message(const ZstEntityBase * entity) override;
	ZstMessage * init_message(ZstMsgKind kind) override;
	ZstMessage * init_serialisable_message(ZstMsgKind kind, const ZstSerialisable & streamable) override;

	void unpack(void * msg) override;

	zmsg_t * handle();

private:
	ZstCZMQMessage();

	ZstMsgKind unpack_kind();
	ZstMsgKind unpack_kind(zframe_t * kind_frame);

	void append_kind_frame(ZstMsgKind k) override;
	void append_id_frame() override;
	void append_payload_frame(const ZstSerialisable & streamable) override;
	void append_str(const char * s, size_t len) override;
	void append_serialisable(ZstMsgKind k, const ZstSerialisable & s) override;

	zmsg_t * m_msg_handle;
};

