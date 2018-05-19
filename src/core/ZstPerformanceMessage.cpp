#include "ZstPerformanceMessage.h"

ZstPerformanceMessage * ZstPerformanceMessage::init_performance_message(ZstOutputPlug * plug)
{
	this->append_str(plug->URI().path(), plug->URI().full_size());
	this->append_payload_frame(*(plug_raw_value(plug)));
	return this;
}

void ZstPerformanceMessage::unpack(zmsg_t * msg){
	ZstMessage::unpack(msg);

	//Unpack sender
	m_sender = ZstURI(zmsg_popstr(msg));

	//Unpack value
	ZstMessagePayload p(ZstMsgKind::PLUG_VALUE, zmsg_pop(msg));
	m_payloads.push_back(std::move(p));
}

const ZstURI & ZstPerformanceMessage::sender(){
	return m_sender;
}
