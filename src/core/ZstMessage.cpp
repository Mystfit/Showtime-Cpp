#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <msgpack.hpp>
#include <entities/ZstComponent.h>
#include <entities/ZstContainer.h>
#include <entities/ZstPerformer.h>
#include <ZstLogging.h>
#include "ZstMessage.h"

ZstMessage::ZstMessage() : 
	m_msg_handle(NULL),
	m_msg_kind(ZstMsgKind::EMPTY)
{
}

ZstMessage::~ZstMessage()
{
	if(m_msg_handle)
		zmsg_destroy(&m_msg_handle);
}

ZstMessage::ZstMessage(const ZstMessage & other)
{
	m_payloads = other.m_payloads;
	m_msg_handle = zmsg_dup(other.m_msg_handle);
}

void ZstMessage::init()
{
	m_msg_handle = zmsg_new();
	this->reset();
}

ZstMessage * ZstMessage::init(ZstMsgKind kind)
{
	this->append_kind_frame(kind);
	return this;
}

ZstMessage * ZstMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	this->append_kind_frame(kind);
	this->append_args(args);
	return this;
}

ZstMessage * ZstMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable)
{
	this->append_kind_frame(kind);
	this->append_args({ {} });
	this->append_payload_frame(serialisable);
	return this;
}

ZstMessage * ZstMessage::init(ZstMsgKind kind, const ZstSerialisable & serialisable, const ZstMsgArgs & args)
{
	this->append_kind_frame(kind);
	this->append_args(args);
	this->append_payload_frame(serialisable);
	return this;
}

void ZstMessage::reset()
{
	m_msg_id = ZstMsgIDManager::next_id();
	m_payloads.clear();
}

void ZstMessage::set_inactive()
{
	m_msg_handle = NULL;
}

void ZstMessage::unpack(zmsg_t * msg)
{
	//Unpack kind
	zframe_t * kind_frame = zmsg_pop(msg);
	auto handle = msgpack::unpack((char*)zframe_data(kind_frame), zframe_size(kind_frame));
	m_msg_kind = handle.get().as<ZstMsgKind>(); 
	zframe_destroy(&kind_frame);

	//Unpack args (optional)
	zframe_t * args_frame = zmsg_pop(msg);
	if (!args_frame) return;
	handle = msgpack::unpack((char*)zframe_data(args_frame), zframe_size(args_frame));
	m_args = handle.get().as<ZstMsgArgs>();
	if (m_args.size() > 0) {
		if (m_args.begin()->first.size() < 1 && m_args.begin()->second.size() == 0) {
			//Empty items in map, clear it.
			m_args.clear();
		}
	}

	//Unpack payload (optional)
	zframe_t * payload_frame = zmsg_pop(msg);
	if (!payload_frame) return;
	while (payload_frame != NULL) {
		m_payloads.emplace_back(payload_frame);
		payload_frame = zmsg_pop(msg);
	}
}

ZstMsgKind ZstMessage::unpack_kind(zframe_t * kind_frame)
{
	ZstMsgKind k = ZstMsgKind::EMPTY;
	
	return k;
}

void ZstMessage::append_args(const ZstMsgArgs & args)
{
	m_args = args;
	std::stringstream buffer;
	msgpack::pack(buffer, args);

	zframe_t * str_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(m_msg_handle, &str_frame);
}

const std::string & ZstMessage::get_arg_s(const char * key)
{
	return m_args.at(key);
}

const char * ZstMessage::get_arg(const char * key)
{
	return get_arg_s(key).c_str();
}

size_t ZstMessage::get_arg_size(const char * key)
{
	return get_arg_s(key).size();
}

 void ZstMessage::log_args()
{
	 ZstLog::net(LogLevel::notification, "Message args:");
	 for (auto arg_pair : m_args) {
		 ZstLog::net(LogLevel::notification, "{}: {}", arg_pair.first, arg_pair.second);
	 }
}

ZstMessagePayload & ZstMessage::payload_at(size_t index)
{
	assert(index <= m_payloads.size());
	return m_payloads.at(index);
}

size_t ZstMessage::num_payloads()
{
	return m_payloads.size();
}

zmsg_t * ZstMessage::handle()
{
	return m_msg_handle;
}

//const ZstURI & ZstMessage::sender() const {
//	return m_sender;
//}

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

void ZstMessage::append_kind_frame(ZstMsgKind k)
{
	m_msg_kind = k;

	std::stringstream buffer;
	msgpack::pack(buffer, k);
		
	zframe_t * kind_frame = zframe_new(buffer.str().c_str(), buffer.str().size());
	zmsg_append(handle(), &kind_frame);
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

void ZstMessage::append_serialisable(ZstMsgKind k, const ZstSerialisable & s)
{
	append_kind_frame(k);
	append_payload_frame(s);
}

ZstMsgKind ZstMessage::entity_kind(ZstEntityBase * entity)
{
	//TODO: Replace with single CREATE_COMPONENT
	ZstMsgKind kind(ZstMsgKind::EMPTY);
	if (strcmp(entity->entity_type(), COMPONENT_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_COMPONENT;
	}
	else if (strcmp(entity->entity_type(), CONTAINER_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_CONTAINER;
	}
	else if (strcmp(entity->entity_type(), PERFORMER_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_PERFORMER;
	}
	else if (strcmp(entity->entity_type(), PLUG_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_PLUG;
	}
	return kind;
}

void ZstMessage::append_payload_frame(const ZstSerialisable & streamable)
{
	std::stringstream buffer;
	streamable.write(buffer);

	//Build message
	zmsg_addmem(m_msg_handle, buffer.str().c_str(), buffer.str().size());
}

void ZstMessage::set_handle(zmsg_t * handle){
	m_msg_handle = handle;
}


// -----------------------
// Message payload
// -----------------------

ZstMessagePayload::ZstMessagePayload(zframe_t * p){
	m_payload = p;
	m_size = zframe_size((zframe_t*)m_payload);
}

ZstMessagePayload::ZstMessagePayload(const ZstMessagePayload & other)
{
	m_size = other.m_size;
	m_payload = zframe_dup((zframe_t*)other.m_payload);
}

ZstMessagePayload::ZstMessagePayload(ZstMessagePayload && source) noexcept
{
	//Move values
	m_size = source.m_size;
	m_payload = source.m_payload;

	//Reset original
	source.m_size = 0;
	source.m_payload = NULL;
}

ZstMessagePayload::~ZstMessagePayload(){
	zframe_t * frame = (zframe_t*)m_payload;
	zframe_destroy(&frame);
}


ZstMessagePayload & ZstMessagePayload::operator=(const ZstMessagePayload & other)
{
	m_size = other.m_size;
	m_payload = other.m_payload;
	return *this;
}

const size_t ZstMessagePayload::size()
{
	return m_size;
}

const char * ZstMessagePayload::data(){
	return (char*)zframe_data(m_payload);
}

