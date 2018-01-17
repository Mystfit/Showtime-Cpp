#include <memory>
#include <czmq.h>
#include "ZstMessage.h"

ZstMessage::ZstMessage() :
	m_sender_length(0),
	m_sender(NULL),
	m_msg_handle(NULL),
	m_msg_id(NULL),
	m_msg_kind(Kind::EMPTY),
	m_entity_target(NULL)
{
	reset();
}

ZstMessage::~ZstMessage()
{
	zstr_free(&m_sender);
	zmsg_destroy(&m_msg_handle);
	zuuid_destroy(&m_msg_id);
	m_payloads.clear();
}

void ZstMessage::reset()
{
	m_sender_length = 0;
	m_msg_kind = Kind::EMPTY;

	zstr_free(&m_sender);
	m_sender = NULL;

	m_payloads.clear();

	zmsg_destroy(&m_msg_handle);
	zuuid_destroy(&m_msg_id);
	m_msg_id = NULL;
}

ZstMessage::ZstMessage(const ZstMessage & other)
{
	m_sender = (char*)malloc(m_sender_length + 1);
	m_sender = strncpy(m_sender, other.m_sender, other.m_sender_length);
	m_sender_length = other.m_sender_length;
	m_sender[m_sender_length] = '\0';
	
	m_msg_kind = other.m_msg_kind;
	m_payloads = other.m_payloads;
	m_msg_handle = zmsg_dup(other.m_msg_handle);
	m_msg_id = zuuid_dup(m_msg_id);
}

void ZstMessage::copy_id(const ZstMessage * msg)
{
	zuuid_set(m_msg_id, zuuid_data(msg->m_msg_id));
}

ZstMessage * ZstMessage::init_entity_message(ZstEntityBase * entity)
{
	zmsg_destroy(&m_msg_handle);
	m_msg_handle = zmsg_new();
	append_id_frame();
	append_entity_kind_frame(entity);
	append_payload_frame(*entity);
	m_entity_target = entity;
	return this;
}

ZstMessage * ZstMessage::init_message(Kind kind)
{
	zmsg_destroy(&m_msg_handle);
	m_msg_handle = zmsg_new();
	append_id_frame();
	append_kind_frame(kind);
	return this;
}

ZstMessage * ZstMessage::init_streamable_message(Kind kind, ZstStreamable & streamable)
{
	zmsg_destroy(&m_msg_handle);
	m_msg_handle = zmsg_new();
	append_id_frame();
	append_kind_frame(kind);
	append_payload_frame(streamable);
	return this;
}

ZstEntityBase * ZstMessage::entity_target()
{
	return m_entity_target;
}

zmsg_t * ZstMessage::handle()
{
	return m_msg_handle;
}

size_t ZstMessage::sender_length()
{
	return m_sender_length;
}

const char * ZstMessage::sender()
{
	return m_sender;
}

ZstURI ZstMessage::sender_as_URI()
{
	return ZstURI(m_sender);
}

const char * ZstMessage::id()
{
	return zuuid_str(m_msg_id);
}

ZstMessage::Kind ZstMessage::kind()
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
	std::stringstream buffer;
	streamable.write(buffer);

	//Build message
	zmsg_addmem(m_msg_handle, buffer.str().c_str(), buffer.str().size());
}

//Build a message id from the message ID enum
void ZstMessage::append_kind_frame(Kind k) {
	char id[sizeof(Kind)];
	sprintf(id, "%d", (int)k);
	zframe_t * kind_frame = zframe_from(id);
	zmsg_append(m_msg_handle, &kind_frame);
}

void ZstMessage::append_id_frame()
{
	m_msg_id = zuuid_new();
	zmsg_addmem(m_msg_handle, zuuid_data(m_msg_id), zuuid_size(m_msg_id));
}

void ZstMessage::append_str(const char * s)
{
	zmsg_addstr(m_msg_handle, s);
}

void ZstMessage::append_streamable(ZstMessage::Kind k,  ZstStreamable & s)
{
	if (kind() == Kind::GRAPH_SNAPSHOT) {
		append_kind_frame(k);
	}
	append_payload_frame(s);
}

void ZstMessage::unpack(zmsg_t * msg)
{
	m_msg_handle = msg;
	zframe_t * first_frame = zmsg_pop(m_msg_handle);
	zframe_t * id_frame = NULL;

	//Unpack Identity if we have one
	if (zframe_size(first_frame) > KIND_FRAME_SIZE) {
		zframe_t * empty = zmsg_pop(m_msg_handle);
		m_sender_length = zframe_size(first_frame);
		m_sender = (char*)malloc(m_sender_length + 1);
		memcpy(m_sender, (char*)zframe_data(first_frame), m_sender_length);
		m_sender[m_sender_length] = '\0';
		zmsg_append(m_msg_handle, &first_frame);
		zmsg_append(m_msg_handle, &empty);

		//Get the next frame (will be ID)
		id_frame = zmsg_pop(m_msg_handle);
	}
	else {
		id_frame = first_frame;
	}

	//Unpack id
	m_msg_id = zuuid_new_from(zframe_data(id_frame));
	zmsg_append(m_msg_handle, &id_frame);

	//Unpack kind
	m_msg_kind = unpack_kind();

	//Handle message payloads
	if (kind() == Kind::GRAPH_SNAPSHOT) {
		// Batched update messages from the stage look like this:
		// | Kind | Payload | Kind | Payload | ... |
		Kind payload_kind = unpack_kind();
		while (payload_kind != Kind::EMPTY) {
			m_payloads.push_back(ZstMessagePayload{ payload_kind, zmsg_pop(m_msg_handle) });
			payload_kind = unpack_kind();
		}
	} else {
		// Normal payloads don't have kind frames and look like this
		// | Payload | Payload | Payload | ... |
		zframe_t * payload_frame = zmsg_pop(m_msg_handle);
		while (payload_frame) {
			m_payloads.push_back(ZstMessagePayload{ m_msg_kind, payload_frame });
			payload_frame = zmsg_pop(m_msg_handle);
		}
	}
}

ZstMessage::Kind ZstMessage::unpack_kind()
{
	return unpack_kind(zmsg_pop(m_msg_handle));
}

ZstMessage::Kind ZstMessage::unpack_kind(zframe_t * kind_frame)
{
	Kind k = Kind::EMPTY;
	if (kind_frame) {
		k = (ZstMessage::Kind)std::atoi((char*)zframe_data(kind_frame));
		zmsg_append(m_msg_handle, &kind_frame);
	}
	return k;
}



// -----------------------
// Message payload wrapper
// -----------------------

ZstMessagePayload::ZstMessagePayload(ZstMessage::Kind k, zframe_t * p)
{
	m_kind = k;
	m_payload = p;
}

ZstMessagePayload::ZstMessagePayload(const ZstMessagePayload & other)
{
	m_kind = other.m_kind;
	m_payload = zframe_dup(other.m_payload);
}

ZstMessagePayload::~ZstMessagePayload()
{
	zframe_destroy(&m_payload);
}

size_t ZstMessagePayload::size()
{
	return zframe_size(m_payload);
}

char * ZstMessagePayload::data()
{
	return (char*)zframe_data(m_payload);
}
