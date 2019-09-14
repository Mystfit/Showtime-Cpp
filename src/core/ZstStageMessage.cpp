#include <boost/uuid/nil_generator.hpp>

#include "ZstStageMessage.h"

#include "entities/ZstEntityBase.h"
#include "entities/ZstComponent.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstPlug.h"
#include "entities/ZstEntityFactory.h"

namespace showtime {

ZstStageMessage::ZstStageMessage() :
	m_endpoint_UUID(nil_generator()())
{
}

ZstStageMessage::ZstStageMessage(const ZstStageMessage & other) : 
	m_msg_args(other.m_msg_args),
	m_endpoint_UUID(other.m_endpoint_UUID)
{
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

ZstMessage * ZstStageMessage::init(ZstMsgKind kind, const ZstMsgArgs& args)
{
	reset();
	set_kind(kind);
	set_args(args);
	return this;
}

ZstMessage* ZstStageMessage::init(ZstMsgKind kind, const ZstMsgArgs& args, const ZstMsgPayload & payload)
{
	reset();
	set_kind(kind);
	set_args(args);
	set_payload(payload);
	return this;
}


void ZstStageMessage::reset() {
	m_msg_args = ZstMsgArgs();
	m_payload = ZstMsgPayload();
	m_endpoint_UUID = nil_generator()();
	set_id(ZstMsgIDManager::next_id());
}

void ZstStageMessage::unpack(const json & data)
{
	m_msg_args = data;
	try {
		m_payload = data.at(get_msg_arg_name(ZstMsgArg::PAYLOAD)).get<ZstMsgPayload>();
	}
	catch (nlohmann::detail::out_of_range) {}

	//Remove duplicate payload key
	m_msg_args.erase(get_msg_arg_name(ZstMsgArg::PAYLOAD));
}

ZstMsgKind ZstStageMessage::entity_kind(const ZstEntityBase & entity)
{
	//TODO: Replace with single CREATE_COMPONENT
	ZstMsgKind kind(ZstMsgKind::EMPTY);
	if (entity.entity_type() == EntityType_COMPONENT) {
		kind = ZstMsgKind::CREATE_COMPONENT;
	}
	else if (entity.entity_type() == EntityType_PERFORMER) {
		kind = ZstMsgKind::CREATE_PERFORMER;
	}
	else if (entity.entity_type() == EntityType_PLUG) {
		kind = ZstMsgKind::CREATE_PLUG;
	}
	else if (entity.entity_type() == EntityType_FACTORY) {
		kind = ZstMsgKind::CREATE_FACTORY;
	}
	return kind;
}

void ZstStageMessage::set_endpoint_UUID(const uuid& uuid)
{
	m_endpoint_UUID = boost::uuids::uuid(uuid);
}

const uuid& ZstStageMessage::endpoint_UUID() const
{
	return m_endpoint_UUID;
}

void ZstStageMessage::set_payload(const ZstMsgPayload& payload)
{
	m_payload = payload;
}

void ZstStageMessage::set_args(const ZstMsgArgs& args)
{
	if (!args.is_object())
		return;
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

ZstMsgKind ZstStageMessage::kind() const
{
	return get_msg_kind(m_msg_args.at(get_msg_arg_name(ZstMsgArg::KIND)).get<std::string>());
}

const ZstMsgPayload& ZstStageMessage::payload() const
{
	return m_payload;
}

ZstMsgID ZstStageMessage::id() const
{
	return m_msg_args.at(get_msg_arg_name(ZstMsgArg::MSG_ID)).get<ZstMsgID>();
}

void ZstStageMessage::set_id(const ZstMsgID & id)
{
	m_msg_args[get_msg_arg_name(ZstMsgArg::MSG_ID)] = id;
}

std::string ZstStageMessage::as_json_str() const
{
	json with_payload = m_msg_args;
	with_payload[get_msg_arg_name(ZstMsgArg::PAYLOAD)] = m_payload;
	return with_payload.dump();
}

}
