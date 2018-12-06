#pragma once

#include <vector>
#include "ZstMessage.h"

class ZstPerformanceMessage : public ZstMessage {
public:
    ZST_EXPORT virtual ~ZstPerformanceMessage();
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args) override;

	ZST_EXPORT virtual void reset() override;

	ZST_EXPORT virtual void unpack(const char * data, const size_t & size);

	ZST_EXPORT virtual const ZstMsgArgs & payload() const override;
	ZST_EXPORT std::string sender() const;
	ZST_EXPORT std::vector<uint8_t> to_msgpack() const;


private:
	void set_payload(const ZstMsgArgs & payload);
	void set_sender(const std::string & sender);

	json m_args;
};
