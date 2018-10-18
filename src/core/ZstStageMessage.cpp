#include "ZstStageMessage.h"

ZstStageMessage::ZstStageMessage()
{
	//Need to reset this message to get it ready for sending
}

ZstStageMessage::ZstStageMessage(const ZstStageMessage & other){
	m_msg_id = other.m_msg_id;
    m_msg_kind = other.m_msg_kind;
}

ZstStageMessage::~ZstStageMessage(){
}

void ZstStageMessage::reset(){
	ZstMessage::reset();
	m_msg_kind = ZstMsgKind::EMPTY;
}

ZstStageMessage * ZstStageMessage::init(ZstMsgKind kind)
{
	ZstMessage::init();
	this->append_id_frame(m_msg_id);
	ZstMessage::init(kind);
	return this;
}

ZstStageMessage * ZstStageMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	ZstMessage::init();
	this->append_id_frame(m_msg_id);
	ZstMessage::init(kind, args);
	return this;
}

ZstStageMessage * ZstStageMessage::init(ZstMsgKind kind, const std::string & payload)
{
	ZstMessage::init();
	this->append_id_frame(m_msg_id);
	ZstMessage::init(kind, payload);
	return this;
}

ZstStageMessage * ZstStageMessage::init(ZstMsgKind kind, const std::string & payload, const ZstMsgArgs & args)
{
	ZstMessage::init();
	this->append_id_frame(m_msg_id);
	ZstMessage::init(kind, payload, args);
	return this;
}

void ZstStageMessage::unpack(zmsg_t * msg)
{
	//Unpack ID
	zframe_t * id_frame = zmsg_pop(msg);
	auto handle = msgpack::unpack((char*)zframe_data(id_frame), zframe_size(id_frame));
	m_msg_id = handle.get().as<ZstMsgID>();
	zframe_destroy(&id_frame);
	ZstMessage::unpack(msg);
}
