#pragma once

#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "ZstMessage.h"

using namespace boost::uuids;

class ZstPerformanceMessage : public ZstMessage {
public:
    ZST_EXPORT virtual ~ZstPerformanceMessage();
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & args, const ZstMsgPayload& payload) override;

	ZST_EXPORT virtual void reset() override;

	ZST_EXPORT virtual const ZstMsgKind& kind() const override;
	ZST_EXPORT virtual ZstMsgID id() const override;
	ZST_EXPORT virtual void set_id(const ZstMsgID& id);

	ZST_EXPORT virtual void unpack(const char * data, const size_t & size);

	ZST_EXPORT virtual const ZstMsgArgs & payload() const override;
	ZST_EXPORT std::string sender() const;
	ZST_EXPORT std::vector<uint8_t> to_msgpack() const;

	ZST_EXPORT const uuid& endpoint_UUID() const override { return nil_generator()(); }
private:
	void set_payload(const ZstMsgArgs & payload);
	void set_sender(const std::string & sender);

	json m_args;
};
