#include <memory>
#include <czmq.h>
#include "ZstMessage.h"

ZstMessage::ZstMessage() : 
	m_sender(NULL),
	m_msg_id(NULL),
	m_msg_handle(zmsg_new()),
	m_msg_kind(Kind::EMPTY),
	m_msg_target(NULL)
{
}

ZstMessage::ZstMessage(zmsg_t * msg) : 
	m_msg_handle(zmsg_new())
{
	unpack(msg);
}

ZstMessage::~ZstMessage()
{
	zmsg_destroy(&m_msg_handle);
	zuuid_destroy(&m_msg_id);
}

void ZstMessage::destroy(ZstMessage * msg)
{
}

ZstMessage ZstMessage::create_entity_message(ZstEntityBase * entity)
{
	ZstMessage msg = ZstMessage();
	msg.append_id_frame();
	msg.append_entity_kind_frame(entity);
	msg.append_payload_frame(*entity);
	return msg;
}

ZstMessage ZstMessage::create_signal_message(Kind kind)
{
	ZstMessage msg;
	msg.append_id_frame();
	msg.append_kind_frame(kind);
	return msg;
}

ZstMessage ZstMessage::create_streamable_message(Kind kind, ZstStreamable & streamable)
{
	ZstMessage msg = ZstMessage();
	msg.append_id_frame();
	msg.append_kind_frame(kind);
	msg.append_payload_frame(streamable);
}

void ZstMessage::append_entity_kind_frame(ZstEntityBase * entity) {
	if (strcmp(entity->entity_type(), COMPONENT_TYPE)) {
		m_msg_kind = Kind::CREATE_COMPONENT;
	}
	else if (strcmp(entity->entity_type(), CONTAINER_TYPE)) {
		m_msg_kind = Kind::CREATE_CONTAINER;
	}
	else if (strcmp(entity->entity_type(), PERFORMER_TYPE)) {
		m_msg_kind = Kind::CREATE_PERFORMER;
	}
	else if (strcmp(entity->entity_type(), PLUG_TYPE)) {
		m_msg_kind = Kind::CREATE_PLUG;
	}

	append_kind_frame(m_msg_kind);
}

void ZstMessage::append_payload_frame(ZstStreamable & streamable)
{
	//Package our root container for the stage
	std::stringstream buffer;
	streamable.write(buffer);

	//Build connect message
	zmsg_addmem(m_msg_handle, buffer.str().c_str(), buffer.str().size());
}

void ZstMessage::append_identity_frame()
{
	zmsg_addmem(m_msg_handle, m_sender, m_sender_length);
	zframe_t * empty = zframe_new_empty();
	zmsg_append(m_msg_handle, &empty);
}

//Build a message id from the message ID enum
void ZstMessage::append_kind_frame(Kind msg_id) {
	char id[sizeof(Kind)];
	sprintf(id, "%d", (int)msg_id);
	zframe_t * kind_frame = zframe_from(id);
	zmsg_append(m_msg_handle, &kind_frame);
}

void ZstMessage::append_id_frame()
{
	m_msg_id = zuuid_new();
}

void ZstMessage::unpack(zmsg_t * msg)
{
	zframe_t * first_frame = zmsg_pop(msg);

	//Unpack Identity
	if (zframe_size(first_frame) > KIND_FRAME_SIZE) {
		zframe_t * empty = zmsg_pop(msg);
		m_sender_length = zframe_size(first_frame);
		m_sender = (char*)malloc(m_sender_length + 1);
		memcpy(m_sender, (char*)zframe_data(first_frame), m_sender_length);
		m_sender[m_sender_length] = '\0';
		zmsg_append(m_msg_handle, &first_frame);
		zmsg_append(m_msg_handle, &empty);
	}

	//Unpack id
	zframe_t * id_frame = zmsg_pop(msg);
	m_msg_id = zuuid_new_from(zframe_data(id_frame));
	zmsg_append(m_msg_handle, &id_frame);

	//Unpack kind
	zframe_t * kind_frame = zmsg_pop(msg);
	ZstMessage::Kind k = ZstMessage::Kind::EMPTY;
	m_msg_kind = (ZstMessage::Kind)std::atoi((char*)zframe_data(kind_frame));
	zmsg_append(m_msg_handle, &id_frame);

	//Unpack payload
	zframe_t * payload_frame = zmsg_pop(msg);
	zmsg_append(m_msg_handle, &payload_frame);
}