#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <msgpack.hpp>
#include <entities/ZstEntityFactory.h>
#include <entities/ZstComponent.h>
#include <entities/ZstContainer.h>
#include <entities/ZstPerformer.h>
#include <entities/ZstPlug.h>
#include "ZstMessage.h"

ZstMessage::ZstMessage() : 
	m_msg_kind(ZstMsgKind::EMPTY),
    m_msg_handle(NULL),
	m_payload(NULL),
	m_payload_frame(NULL),
	m_payload_offset(0),
	m_payload_size(0)
{
}

ZstMessage::~ZstMessage()
{
	if(m_msg_handle)
		zmsg_destroy(&m_msg_handle);
}

ZstMessage::ZstMessage(const ZstMessage & other)
{
	m_payload = other.m_payload;
	m_msg_handle = zmsg_dup(other.m_msg_handle);
}

void ZstMessage::init()
{
	m_msg_handle = zmsg_new();
	this->reset();
}

ZstMessage * ZstMessage::init(ZstMsgKind kind)
{
	this->append_kind(kind);
	return this;
}

ZstMessage * ZstMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	this->append_kind(kind);
	this->append_args(args);
	return this;
}

ZstMessage * ZstMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	this->append_kind(kind);
	this->append_empty_args();
	this->append_payload(serialisable);
	return this;
}

ZstMessage * ZstMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args)
{
	this->append_kind(kind);
	this->append_args(args);
	this->append_payload(serialisable);
	return this;
}

void ZstMessage::reset()
{
	m_msg_id = ZstMsgIDManager::next_id();
	m_payload = NULL;
	zframe_destroy(&m_payload_frame);
	m_payload_frame = NULL;
	m_payload_offset = 0;
	m_payload_size = 0;
}

void ZstMessage::set_inactive()
{
	m_msg_handle = NULL;
	m_payload_frame = NULL;
}

void ZstMessage::unpack(zmsg_t * msg)
{
	unpack_next_kind(msg);
	unpack_next_args(msg);
	unpack_next_payload(msg);
}

void ZstMessage::unpack(zframe_t * frame)
{
	char * frame_data = (char*)zframe_data(frame);
	size_t frame_size = zframe_size(frame);
	size_t offset = 0;

	unpack_next_kind(frame_data, frame_size, offset);
	unpack_next_args(frame_data, frame_size, offset);
	unpack_next_payload(frame_data, frame_size, offset);
}


void ZstMessage::append_empty_args()
{
	std::stringstream buffer;
	msgpack::pack(buffer,0);
	zframe_t * frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(m_msg_handle, &frame);
}

void ZstMessage::append_args(const ZstMsgArgs & args)
{
	std::stringstream buffer;
	append_args(args, buffer);
	zframe_t * str_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(m_msg_handle, &str_frame);
}

void ZstMessage::append_args(const ZstMsgArgs & args, std::stringstream & buffer)
{
	m_args = std::move(args);
	msgpack::pack(buffer, args.size());
	if (args.size() > 0) {
		msgpack::pack(buffer, args);
	}
}

void ZstMessage::set_local_arg(const ZstMsgArg & key, const std::string & value)
{
	m_args[key] = value;
}

const std::string & ZstMessage::get_arg_s(const ZstMsgArg & key) const
{
	return m_args.at(key);
}

bool ZstMessage::has_arg(const ZstMsgArg & key) const
{
	return m_args.find(key) != m_args.end();
}

const char * ZstMessage::get_arg(const ZstMsgArg & key) const
{
	return get_arg_s(key).c_str();
}

size_t ZstMessage::get_arg_size(const ZstMsgArg & key) const
{
	return get_arg_s(key).size();
}

const char * ZstMessage::payload_data()
{
	if (m_payload_frame) {
		return (char*)zframe_data(m_payload_frame);
	}
	return m_payload;
}

const size_t ZstMessage::payload_size()
{
	if (m_payload_frame) {
		return zframe_size(m_payload_frame);
	}
	return m_payload_size;
}

size_t & ZstMessage::payload_offset()
{
	return m_payload_offset;
}

zmsg_t * ZstMessage::handle()
{
	return m_msg_handle;
}

zframe_t * ZstMessage::payload_frame()
{
	return m_payload_frame;
}

const ZstMsgKind ZstMessage::kind() const
{
	return m_msg_kind;
}

ZstMsgID ZstMessage::id() const
{
	return m_msg_id;
}

void ZstMessage::set_id(ZstMsgID id)
{
	m_msg_id = id;
	
	//Remove old id from front of message
	zframe_t * old_id_frame = zmsg_pop(handle());
	zframe_destroy(&old_id_frame);

	//Add new frame to front of message
	prepend_id_frame(id);
}

void ZstMessage::copy_id(const ZstMessage * msg)
{
	this->set_id(msg->id());
}

void ZstMessage::append_kind(ZstMsgKind k)
{
	std::stringstream buffer;
	append_kind(k, buffer);
	zframe_t * kind_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(handle(), &kind_frame);
}

void ZstMessage::append_kind(ZstMsgKind k, std::stringstream & buffer)
{
	m_msg_kind = k;
	msgpack::pack(buffer, k);
}

void ZstMessage::prepend_id_frame(ZstMsgID id)
{
	std::stringstream buffer;
	msgpack::pack(buffer, id);
	zmsg_pushmem(handle(), buffer.str().c_str(), buffer.str().size());
}

void ZstMessage::append_id_frame(ZstMsgID id)
{
	std::stringstream buffer;
	msgpack::pack(buffer, id);
	zmsg_addmem(handle(), buffer.str().c_str(), buffer.str().size());
}

ZstMsgKind ZstMessage::entity_kind(const ZstEntityBase & entity)
{
	//TODO: Replace with single CREATE_COMPONENT
	ZstMsgKind kind(ZstMsgKind::EMPTY);
	if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_COMPONENT;
	}
	else if (strcmp(entity.entity_type(), CONTAINER_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_CONTAINER;
	}
	else if (strcmp(entity.entity_type(), PERFORMER_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_PERFORMER;
	}
	else if (strcmp(entity.entity_type(), PLUG_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_PLUG;
	}
	else if (strcmp(entity.entity_type(), FACTORY_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_FACTORY;
	}
	return kind;
}

void ZstMessage::append_payload(const ZstSerialisable & streamable)
{
	std::stringstream buffer;
	append_payload(streamable, buffer);
	zmsg_addmem(m_msg_handle, buffer.str().c_str(), buffer.str().size());
}

void ZstMessage::append_payload(const ZstSerialisable & streamable, std::stringstream & buffer)
{
	streamable.write(buffer);
}

void ZstMessage::set_handle(zmsg_t * handle){
	m_msg_handle = handle;
}

void ZstMessage::unpack_next_kind(zmsg_t * msg)
{
	//Unpack kind
	zframe_t * frame = zmsg_pop(msg);
	assert(frame);
	size_t offset = 0;
	unpack_next_kind((const char*)zframe_data(frame), zframe_size(frame), offset);

	//Cleanup kind frame
	zframe_destroy(&frame);
}

void ZstMessage::unpack_next_kind(const char * data, size_t size, size_t & offset)
{
	msgpack::object_handle handle = msgpack::unpack(data, size, offset);
	m_msg_kind = handle.get().as<ZstMsgKind>();
}

void ZstMessage::unpack_next_args(zmsg_t * msg)
{
	//Unpack args (optional)
	zframe_t * frame = zmsg_pop(msg);
	if (!frame) return;
	size_t offset = 0;
	unpack_next_args((char*)zframe_data(frame), zframe_size(frame), offset);
	zframe_destroy(&frame);
}

void ZstMessage::unpack_next_args(const char * data, size_t size, size_t & offset)
{
	msgpack::object_handle handle = msgpack::unpack(data, size, offset);
	size_t num_args = static_cast<size_t>(handle.get().via.i64);

	//Only unpack arguments if we have any
	if (num_args > 0) {
		handle = msgpack::unpack(data, size, offset);
		m_args = handle.get().as<ZstMsgArgs>();
	}
}

void ZstMessage::unpack_next_payload(zmsg_t * msg)
{
	//Unpack payload (optional)
	zframe_t * frame = zmsg_pop(msg);
	if (!frame) return;
	m_payload_frame = frame;
}

void ZstMessage::unpack_next_payload(char * data, size_t size, size_t & offset)
{
	m_payload = data;
	m_payload_size = size;
	m_payload_offset = offset;
}
