#pragma once

#include "ZstExports.h"
#include "ZstMessage.h"
#include <nlohmann/json.hpp>
#include "entities/ZstEntityBase.h"

using nlohmann::json;

class ZST_EXPORT ZstStageMessage : public ZstMessage {
public:
    ZST_EXPORT ZstStageMessage();
    ZST_EXPORT ZstStageMessage(const ZstStageMessage & other);
    ZST_EXPORT virtual ~ZstStageMessage();

	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstMessage * init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args) override;
	
	ZST_EXPORT void reset() override;
	
	ZST_EXPORT virtual const ZstMsgKind& kind() const override;
	ZST_EXPORT virtual ZstMsgID id() const override;
	ZST_EXPORT virtual void set_id(const ZstMsgID& id) override;

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
		return m_msg_args.at(get_msg_arg_name(key)).get<T>();
	}

	template<typename K, typename T>
	void set_arg(K key, T val) {
		m_msg_args[key] = val;
	}

	ZST_EXPORT virtual const ZstMsgArgs & payload() const override;
	ZST_EXPORT std::string as_json_str() const;

private:
	ZST_EXPORT void set_payload(const ZstMsgArgs & payload);
	ZST_EXPORT void set_payload(const std::string & payload);
	ZST_EXPORT void set_args(const ZstMsgArgs & args);
	ZST_EXPORT void set_kind(const ZstMsgKind & k);

	//Message info
	json m_msg_args;
};
