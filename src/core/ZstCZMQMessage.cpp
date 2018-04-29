#include "ZstCZMQMessage.h"


// ---------------------
// ZstCZMQMessagePayload
// ---------------------

ZstCZMQMessagePayload::ZstCZMQMessagePayload(ZstMsgKind k, const zframe_t * p) :
	ZstMessagePayload(k, p)
{
}

ZstCZMQMessagePayload::ZstCZMQMessagePayload(const ZstCZMQMessagePayload & other) : 
	ZstMessagePayload(other)
{
	m_payload = zframe_dup((zframe_t*)other.m_payload);
}

ZstCZMQMessagePayload::~ZstCZMQMessagePayload()
{
	zframe_t * frame = (zframe_t*)m_payload;
	zframe_destroy(&frame);
}

const void * ZstCZMQMessagePayload::data()
{
	return (void*)zframe_data((zframe_t*)m_payload);
}

const size_t ZstCZMQMessagePayload::size() {
	return zframe_size((zframe_t*)m_payload);
}


// --------------
// ZstCZMQMessage
// --------------

ZstCZMQMessage::ZstCZMQMessage() : 
	m_msg_handle(NULL)
{
}

ZstCZMQMessage::~ZstCZMQMessage()
{
	zmsg_destroy(&m_msg_handle);
	delete m_msg_id;
}

ZstCZMQMessage::ZstCZMQMessage(const ZstCZMQMessage & other) : ZstMessage(other)
{
	m_payloads = other.m_payloads;
	m_msg_handle = zmsg_dup(other.m_msg_handle);
}

void ZstCZMQMessage::reset()
{
	m_payloads.clear();
	if (m_msg_handle)
		zmsg_destroy(&m_msg_handle);
	m_msg_handle = zmsg_new();
}

void ZstCZMQMessage::copy_id(const ZstMessage * msg)
{
	ZstMessage::copy_id(msg);

	//Remove old id from front of message
	zframe_t * old_id_frame = zmsg_pop(m_msg_handle);

	zframe_destroy(&old_id_frame);

	//Add new id to front of message
	zmsg_pushmem(m_msg_handle, m_msg_id, UUID_LENGTH);
}

ZstMessagePayload & ZstCZMQMessage::payload_at(size_t index)
{
	return m_payloads.at(index);
}

size_t ZstCZMQMessage::num_payloads()
{
	return m_payloads.size();
}

void ZstCZMQMessage::unpack(void * msg)
{
	//Make sure we are providing a zmsg_t
	assert(zmsg_is(msg));

	m_msg_handle = (zmsg_t*)msg;
	zframe_t * id_frame = zmsg_pop(m_msg_handle);

	//Unpack id
	memcpy(m_msg_id, zframe_data(id_frame), UUID_LENGTH);
	zframe_destroy(&id_frame);

	//Unpack kind
	m_msg_kind = unpack_kind();
	
	//Handle message payloads
	if (kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
		// Batched update messages from the stage look like this:
		// | Kind | Payload | Kind | Payload | ... |
		ZstMsgKind payload_kind = unpack_kind();
		while (payload_kind != ZstMsgKind::EMPTY) {
			m_payloads.push_back(ZstCZMQMessagePayload(payload_kind, zmsg_pop(m_msg_handle)));
			payload_kind = unpack_kind();
		}
	}
	else {
		// Normal payloads don't have kind frames and look like this
		// | Payload | Payload | Payload | ... |
		zframe_t * payload_frame = zmsg_pop(m_msg_handle);
		while (payload_frame){
			ZstCZMQMessagePayload p(m_msg_kind, payload_frame);
			m_payloads.push_back(std::move(p));
			payload_frame = zmsg_pop(m_msg_handle);
		}
	}
}

zmsg_t * ZstCZMQMessage::handle()
{
	return m_msg_handle;
}


ZstMsgKind ZstCZMQMessage::unpack_kind()
{
	zframe_t * kind_frame = zmsg_pop(m_msg_handle);
	ZstMsgKind k = unpack_kind(kind_frame);
	zframe_destroy(&kind_frame);
	return k;
}

ZstMsgKind ZstCZMQMessage::unpack_kind(zframe_t * kind_frame)
{
	ZstMsgKind k = ZstMsgKind::EMPTY;
	if (kind_frame) {
		auto handle = msgpack::unpack((char*)zframe_data(kind_frame), zframe_size(kind_frame));
		k = handle.get().as<ZstMsgKind>();
	}
	return k;
}

void ZstCZMQMessage::append_kind_frame(ZstMsgKind k)
{
	m_msg_kind = k;

	std::stringstream buffer;
	msgpack::pack(buffer, k);

	zframe_t * kind_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(m_msg_handle, &kind_frame);
}

void ZstCZMQMessage::append_id_frame()
{
	zuuid_t * uuid = zuuid_new();
	memcpy(m_msg_id, zuuid_str(uuid), UUID_LENGTH);
	zmsg_addmem(m_msg_handle, m_msg_id, UUID_LENGTH);
}

void ZstCZMQMessage::append_payload_frame(const ZstSerialisable & streamable)
{
	std::stringstream buffer;
	streamable.write(buffer);

	//Build message
	zmsg_addmem(m_msg_handle, buffer.str().c_str(), buffer.str().size());
}

void ZstCZMQMessage::append_str(const char * s, size_t len)
{
	zframe_t * str_frame = zframe_new(s, len);
	zmsg_append(m_msg_handle, &str_frame);
}

void ZstCZMQMessage::append_serialisable(ZstMsgKind k, const ZstSerialisable & s)
{
	if (kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
		append_kind_frame(k);
	}
	append_payload_frame(s);
}
