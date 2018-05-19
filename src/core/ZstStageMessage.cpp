#include "ZstStageMessage.h"
#include <entities/ZstComponent.h>
#include <entities/ZstContainer.h>
#include <entities/ZstPerformer.h>

ZstStageMessage::ZstStageMessage() : m_msg_kind(ZstMsgKind::EMPTY)
{
}

ZstStageMessage::ZstStageMessage(const ZstStageMessage & other){
    memcpy(m_msg_id, other.m_msg_id, ZSTMSG_UUID_LENGTH);
    m_msg_kind = other.m_msg_kind;
}

ZstStageMessage::~ZstStageMessage(){
    memset(m_msg_id, 0, ZSTMSG_UUID_LENGTH);
}

void ZstStageMessage::reset(){
    memset(&m_msg_id[0], 0, ZSTMSG_UUID_LENGTH);
	m_msg_kind = ZstMsgKind::EMPTY;
}

ZstStageMessage * ZstStageMessage::init_entity_message(const ZstEntityBase * entity)
{
	this->append_id_frame();
	this->append_entity_kind_frame(entity);
	this->append_payload_frame(*entity);
	return this;
}

ZstStageMessage * ZstStageMessage::init_message(ZstMsgKind kind)
{
	this->append_id_frame();
	this->append_kind_frame(kind);
	return this;
}

ZstStageMessage * ZstStageMessage::init_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	this->append_id_frame();
	this->append_kind_frame(kind);
	this->append_payload_frame(serialisable);
	return this;
}

void ZstStageMessage::copy_id(const ZstStageMessage * msg)
{
    memcpy(m_msg_id, msg->m_msg_id, ZSTMSG_UUID_LENGTH);

	//Remove old id from front of message
	zframe_t * old_id_frame = zmsg_pop(handle());

	zframe_destroy(&old_id_frame);

	//Add new id to front of message
	zmsg_pushmem(handle(), m_msg_id, ZSTMSG_UUID_LENGTH);
}

void ZstStageMessage::append_entity_kind_frame(const ZstEntityBase * entity) {
	if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0) {
		m_msg_kind = ZstMsgKind::CREATE_COMPONENT;
	}
	else if (strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
		m_msg_kind = ZstMsgKind::CREATE_CONTAINER;
	}
	else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
		m_msg_kind = ZstMsgKind::CREATE_PERFORMER;
	}
	else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		m_msg_kind = ZstMsgKind::CREATE_PLUG;
	}

	append_kind_frame(m_msg_kind);
}

void ZstStageMessage::append_kind_frame(ZstMsgKind k)
{
	m_msg_kind = k;

	std::stringstream buffer;
	msgpack::pack(buffer, k);

	zframe_t * kind_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(handle(), &kind_frame);
}

void ZstStageMessage::append_id_frame()
{
	zuuid_t * uuid = zuuid_new();
	memcpy(m_msg_id, zuuid_str(uuid), ZSTMSG_UUID_LENGTH);
	zmsg_addmem(handle(), m_msg_id, ZSTMSG_UUID_LENGTH);
}


void ZstStageMessage::append_serialisable(ZstMsgKind k, const ZstSerialisable & s)
{
	if (kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
		append_kind_frame(k);
	}
	append_payload_frame(s);
}


void ZstStageMessage::unpack(zmsg_t * msg)
{
	ZstMessage::unpack(msg);

	//Unpack ID
    zframe_t * id_frame = zmsg_pop(handle());
    memcpy(m_msg_id, zframe_data(id_frame), ZSTMSG_UUID_LENGTH);
    zframe_destroy(&id_frame);

	//Unpack kind
	m_msg_kind = unpack_kind();
	
	//Handle message payloads
	if (kind() == ZstMsgKind::GRAPH_SNAPSHOT) {
		// Batched update messages from the stage look like this:
		// | Kind | Payload | Kind | Payload | ... |
		ZstMsgKind payload_kind = unpack_kind();
		while (payload_kind != ZstMsgKind::EMPTY) {
			m_payloads.push_back(ZstMessagePayload(payload_kind, zmsg_pop(handle())));
			payload_kind = unpack_kind();
		}
	}
	else {
		// Normal payloads don't have kind frames and look like this
		// | Payload | Payload | Payload | ... |
		zframe_t * payload_frame = zmsg_pop(handle());
		while (payload_frame){
			ZstMessagePayload p(m_msg_kind, payload_frame);
			m_payloads.push_back(std::move(p));
			payload_frame = zmsg_pop(handle());
		}
	}
}

ZstMsgKind ZstStageMessage::unpack_kind()
{
	zframe_t * kind_frame = zmsg_pop(handle());
	ZstMsgKind k = unpack_kind(kind_frame);
	zframe_destroy(&kind_frame);
	return k;
}

ZstMsgKind ZstStageMessage::unpack_kind(zframe_t * kind_frame)
{
	ZstMsgKind k = ZstMsgKind::EMPTY;
	if (kind_frame) {
		auto handle = msgpack::unpack((char*)zframe_data(kind_frame), zframe_size(kind_frame));
		k = handle.get().as<ZstMsgKind>();
	}
	return k;
}

const char * ZstStageMessage::id()
{
	return m_msg_id;
}

ZstMsgKind ZstStageMessage::kind()
{
	return m_msg_kind;
}

