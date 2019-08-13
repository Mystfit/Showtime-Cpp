#include "ZstStageMessage.h"

#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstPlug.h"
#include "entities/ZstEntityFactory.h"

ZstStageMessage::ZstStageMessage()
{
}

ZstStageMessage::ZstStageMessage(const ZstStageMessage & other)
{
	m_msg_args = other.m_msg_args;
}

ZstStageMessage::~ZstStageMessage()
{
}

ZstMessage * ZstStageMessage::init(ZstMsgKind kind)
{
	reset();
	set_kind(kind);
	return this;
}

ZstMessage * ZstStageMessage::init(ZstMsgKind kind, const ZstMsgArgs & args)
{
	reset();
	set_kind(kind);
	set_args(args);
	return this;
}

ZstMessage * ZstStageMessage::init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args)
{
	reset();
	set_kind(kind);
	set_args(args);
	set_payload(payload);
	return this;
}

void ZstStageMessage::reset() {
	m_msg_args.clear();
	set_id(ZstMsgIDManager::next_id());
}

void ZstStageMessage::unpack(const json & data)
{
	m_msg_args = data;
}

ZstMsgKind ZstStageMessage::entity_kind(const ZstEntityBase & entity)
{
	//TODO: Replace with single CREATE_COMPONENT
	ZstMsgKind kind(ZstMsgKind::EMPTY);
	if (strcmp(entity.entity_type(), COMPONENT_TYPE) == 0) {
		kind = ZstMsgKind::CREATE_COMPONENT;
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


void ZstStageMessage::set_payload(const ZstMsgArgs & payload)
{
	m_msg_args[get_msg_arg_name(ZstMsgArg::PAYLOAD)] = payload;
}

void ZstStageMessage::set_payload(const std::string & payload)
{
	m_msg_args[get_msg_arg_name(ZstMsgArg::PAYLOAD)] = payload;
}

void ZstStageMessage::set_args(const ZstMsgArgs & args)
{
	if(args.is_object())
		m_msg_args.update(args);
}

void ZstStageMessage::set_kind(const ZstMsgKind &  k)
{
	m_msg_args[get_msg_arg_name(ZstMsgArg::KIND)] = get_msg_name(k);
}

bool ZstStageMessage::has_arg(const ZstMsgArg & key) const
{
	return m_msg_args.find(get_msg_arg_name(key)) != m_msg_args.end();
}

const ZstMsgKind & ZstStageMessage::kind() const
{
	return get_msg_kind(m_msg_args.at(get_msg_arg_name(ZstMsgArg::KIND)).get<std::string>());
}

const ZstMsgArgs & ZstStageMessage::payload() const
{
	return m_msg_args[get_msg_arg_name(ZstMsgArg::PAYLOAD)];
}

ZstMsgID ZstStageMessage::id() const
{
	return m_msg_args[get_msg_arg_name(ZstMsgArg::MSG_ID)].get<ZstMsgID>();
}

void ZstStageMessage::set_id(const ZstMsgID & id)
{
	m_msg_args[get_msg_arg_name(ZstMsgArg::MSG_ID)] = id;
}

std::string ZstStageMessage::as_json_str() const
{
	return m_msg_args.dump();
}

