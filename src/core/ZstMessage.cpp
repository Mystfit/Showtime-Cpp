#include <memory>
#include "ZstMessage.h"

ZstMessage::ZstMessage() : m_msg_handle(NULL)
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
}

void ZstMessage::reset()
{
	m_payloads.clear();
}

void ZstMessage::set_inactive()
{
	m_msg_handle = NULL;
}

void ZstMessage::unpack(zmsg_t * msg){
	assert(zmsg_is(msg));
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

void ZstMessage::append_str(const char * s, size_t len)
{
	zframe_t * str_frame = zframe_new(s, len);
	zmsg_append(m_msg_handle, &str_frame);
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


ZstMessagePayload & ZstMessagePayload::operator=(ZstMessagePayload & other)
{
	//Copy assignment
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
