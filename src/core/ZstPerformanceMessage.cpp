#include "ZstPerformanceMessage.h"

ZstPerformanceMessage::ZstPerformanceMessage()
{
}

ZstPerformanceMessage * ZstPerformanceMessage::init_performance_message(ZstOutputPlug * plug)
{
	this->init();
	this->reset();
	this->append_str(plug->URI().path(), plug->URI().full_size());
	this->append_payload_frame(*(plug_raw_value(plug)));
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init_performance_message(const ZstURI & sender)
{
	this->init();
	this->reset();
	this->append_str(sender.path(), sender.full_size());
	return this;
}

void ZstPerformanceMessage::unpack(zmsg_t * msg){
	ZstMessage::unpack(msg);

	//Unpack sender
	zframe_t * sender_frame = zmsg_pop(msg);
	m_sender = std::move(ZstURI((char*)zframe_data(sender_frame), zframe_size(sender_frame)));
	zframe_destroy(&sender_frame);

	//Unpack value
	zframe_t * payload = zmsg_pop(msg);
	if(payload){
		m_payloads.emplace_back(payload);
	}
}

const ZstURI & ZstPerformanceMessage::sender(){
	return m_sender;
}
