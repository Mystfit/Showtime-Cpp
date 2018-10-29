#pragma once

#include "ZstMessage.h"
#include <nlohmann/json.hpp>
#include <entities/ZstEntityBase.h>

using nlohmann::json;

class ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other);
    ZST_EXPORT virtual ~ZstStageMessage();

	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args) override;
	
	ZST_EXPORT void reset() override;

	ZST_EXPORT static ZstMsgKind entity_kind(const ZstEntityBase & entity);
	ZST_EXPORT void unpack(const json & data);

	template <typename T>
	T unpack_payload_serialisable() {
		T serialisable;
		serialisable.read_json(m_msg_args.at(get_msg_arg_name(ZstMsgArg::PAYLOAD)));
		return serialisable;
	}

	ZST_EXPORT bool has_arg(const ZstMsgArg & key) const;

	template<typename T>
	T get_arg(ZstMsgArg key) const {
		return m_msg_args[get_msg_arg_name(key)].get<T>();
	}

	template<typename K, typename T>
	void set_arg(K key, T val) {
		m_msg_args[key] = val;
	}

	ZST_EXPORT const ZstMsgKind & kind() const;
	ZST_EXPORT const char * payload_data() const override;
	ZST_EXPORT const size_t payload_size() const override;
	ZST_EXPORT ZstMsgID id() const;
	ZST_EXPORT void set_id(const ZstMsgID & id);
	ZST_EXPORT std::string as_json_str() const;

private:
	ZST_EXPORT void set_payload(const ZstMsgArgs & payload);
	ZST_EXPORT void set_payload(const std::string & payload);
	ZST_EXPORT void set_args(const ZstMsgArgs & args);
	ZST_EXPORT void set_kind(const ZstMsgKind & k);

	//Message info
	json m_msg_args;
};
