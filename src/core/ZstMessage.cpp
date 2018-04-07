#include <memory>
#include "ZstMessage.h"

ZstMessage::ZstMessage() :
	m_msg_kind(ZstMsgKind::EMPTY)
{
	reset();
}

ZstMessage::~ZstMessage()
{
	m_payloads.clear();
}

ZstMessage::ZstMessage(const ZstMessage & other)
{
	m_msg_kind = other.m_msg_kind;
	m_payloads = other.m_payloads;
	memcpy(m_msg_id, other.m_msg_id, UUID_LENGTH);
}

void ZstMessage::reset()
{
	memset(&m_msg_id[0], 0, UUID_LENGTH);
	m_msg_kind = ZstMsgKind::EMPTY;
	m_payloads.clear();
}

void ZstMessage::copy_id(const ZstMessage * msg)
{
	memcpy(m_msg_id, msg->m_msg_id, UUID_LENGTH);
}

const char * ZstMessage::id()
{
	return m_msg_id;
}

ZstMsgKind ZstMessage::kind()
{
	return m_msg_kind;
}

ZstMessagePayload & ZstMessage::payload_at(size_t index)
{
	return m_payloads.at(index);
}

size_t ZstMessage::num_payloads()
{
	return m_payloads.size();
}

void ZstMessage::append_entity_kind_frame(const ZstEntityBase * entity) {
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


// -----------------------
// Message payload
// -----------------------

ZstMessagePayload::ZstMessagePayload(ZstMsgKind k, const void * p) : 
	m_size(0),
	m_kind(k),
	m_payload(p)
{
}

ZstMessagePayload::ZstMessagePayload(const ZstMessagePayload & other)
{
	m_kind = other.m_kind;
}

ZstMsgKind ZstMessagePayload::kind()
{
	return m_kind;
}

size_t ZstMessagePayload::size()
{
	return m_size;
}

const void * ZstMessagePayload::data()
{
	return m_payload;
}