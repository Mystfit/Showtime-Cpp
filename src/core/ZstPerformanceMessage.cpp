#include "ZstPerformanceMessage.h"

ZstPerformanceMessage::ZstPerformanceMessage()
{
	reset();
}

ZstPerformanceMessage * ZstPerformanceMessage::init_performance_message(ZstOutputPlug * plug)
{
	this->append_str(plug->URI().path(), plug->URI().full_size());
	this->append_payload_frame(*(plug_raw_value(plug)));
	return this;
}

ZstPerformanceMessage * ZstPerformanceMessage::init_performance_message(const ZstURI & sender)
{
	this->append_str(sender.path(), sender.full_size());
	return this;
}

void ZstPerformanceMessage::unpack(zmsg_t * msg){
	ZstMessage::unpack(msg);

	//Unpack sender
	m_sender = ZstURI(zmsg_popstr(msg));

	//Unpack value
	zframe_t * payload = zmsg_pop(msg);
	if(payload){
		ZstMessagePayload p(payload);
		m_payloads.push_back(std::move(p));
	}
}

const ZstURI & ZstPerformanceMessage::sender(){
	return m_sender;
}
