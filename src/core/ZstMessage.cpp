#include "ZstMessage.h"
#include <memory>
#include <czmq.h>
#include "ZstMessage.h"

ZstMessage::ZstMessage() :
	m_msg_handle(NULL),
	m_msg_id(NULL),
	m_msg_kind(Kind::EMPTY),
	m_entity_target(NULL)
{
	reset();
}

ZstMessage::~ZstMessage()
{
	zmsg_destroy(&m_msg_handle);
	zuuid_destroy(&m_msg_id);
	m_payloads.clear();
}

void ZstMessage::reset()
{
	m_msg_kind = Kind::EMPTY;
	m_payloads.clear();
		
	if (m_msg_handle) 
		zmsg_destroy(&m_msg_handle);
	m_msg_handle = NULL;
	
	if (m_msg_id) 
		zuuid_destroy(&m_msg_id);
	m_msg_id = NULL;
}

ZstMessage::ZstMessage(const ZstMessage & other)
{
	m_msg_kind = other.m_msg_kind;
	m_payloads = other.m_payloads;
	m_msg_handle = zmsg_dup(other.m_msg_handle);
	m_msg_id = zuuid_dup(m_msg_id);
}

void ZstMessage::copy_id(const ZstMessage * msg)
{
	assert(m_msg_id != NULL);
	zuuid_destroy(&m_msg_id);
	m_msg_id = zuuid_dup(msg->m_msg_id);

	//Remove old id from front of message
	zframe_t * old_id_frame = zmsg_pop(m_msg_handle);

	zframe_destroy(&old_id_frame);

	//Add new id to front of message
	zmsg_pushmem(m_msg_handle, zuuid_data(m_msg_id), zuuid_size(m_msg_id));
}

ZstMessage * ZstMessage::init_entity_message(ZstEntityBase * entity)
{
	if(m_msg_handle)
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
	if (m_msg_handle)
		zmsg_destroy(&m_msg_handle);
	m_msg_handle = zmsg_new();
	append_id_frame();
	append_kind_frame(kind);
	return this;
}

ZstMessage * ZstMessage::init_serialisable_message(Kind kind, ZstSerialisable & streamable)
{
	if (m_msg_handle)
		zmsg_destroy(&m_msg_handle);
	m_msg_handle = zmsg_new();
	append_id_frame();
	append_kind_frame(kind);
	append_payload_frame(streamable);
	return this;
}

void ZstMessage::send(zsock_t * socket)
{
	zmsg_send(&m_msg_handle, socket);
	m_msg_handle = NULL;
}

ZstEntityBase * ZstMessage::entity_target()
{
	return m_entity_target;
}

zmsg_t * ZstMessage::handle()
{
	return m_msg_handle;
}

const char * ZstMessage::id()
{
	const char * result = NULL;
	if (m_msg_id)
		result = zuuid_str(m_msg_id);
	return result;
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
	if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0) {
		m_msg_kind = Kind::CREATE_COMPONENT;
	}
	else if (strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
		m_msg_kind = Kind::CREATE_CONTAINER;
	}
	else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
		m_msg_kind = Kind::CREATE_PERFORMER;
	}
	else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		m_msg_kind = Kind::CREATE_PLUG;
	}

	append_kind_frame(m_msg_kind);
}

void ZstMessage::append_payload_frame(ZstSerialisable & streamable)
{
	std::stringstream buffer;
	streamable.write(buffer);

	//Build message
	zmsg_addmem(m_msg_handle, buffer.str().c_str(), buffer.str().size());
}

//Build a message id from the message ID enum
void ZstMessage::append_kind_frame(Kind k) {
	m_msg_kind = k;
	
	std::stringstream buffer;
	msgpack::pack(buffer, k);

	zframe_t * kind_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(m_msg_handle, &kind_frame);
}

void ZstMessage::append_id_frame()
{
	m_msg_id = zuuid_new();
	zmsg_addmem(m_msg_handle, zuuid_data(m_msg_id), zuuid_size(m_msg_id));
}

void ZstMessage::append_str(const char * s, size_t len)
{
	zframe_t * str_frame = zframe_new(s, len);
	zmsg_append(m_msg_handle,&str_frame);
}

void ZstMessage::append_serialisable(ZstMessage::Kind k,  ZstSerialisable & s)
{
	if (kind() == Kind::GRAPH_SNAPSHOT) {
		append_kind_frame(k);
	}
	append_payload_frame(s);
}

void ZstMessage::unpack(zmsg_t * msg)
{
	m_msg_handle = msg;
	zframe_t * id_frame = zmsg_pop(m_msg_handle); 
	
	//Unpack id
	m_msg_id = zuuid_new_from(zframe_data(id_frame));
	zframe_destroy(&id_frame);
	
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
	zframe_t * kind_frame = zmsg_pop(m_msg_handle);
	Kind k = unpack_kind(kind_frame);
	zframe_destroy(&kind_frame);
	return k;
}

ZstMessage::Kind ZstMessage::unpack_kind(zframe_t * kind_frame)
{
	Kind k = Kind::EMPTY;
	unsigned int k_int = 0;;
	if (kind_frame) {
		auto handle = msgpack::unpack((char*)zframe_data(kind_frame), zframe_size(kind_frame));
		k = handle.get().as<Kind>();
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
