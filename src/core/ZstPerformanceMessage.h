#pragma once

#include <sstream>
#include "ZstMessage.h"

class ZstPerformanceMessage : public ZstMessage {
public:
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & args) override;
	ZST_EXPORT virtual ZstPerformanceMessage * init(ZstMsgKind kind, const ZstMsgArgs & payload, const ZstMsgArgs & args) override;

	ZST_EXPORT virtual void reset() override;

	ZST_EXPORT virtual void unpack(const char * data, const size_t & size);

	ZST_EXPORT virtual const char * payload_data() const override;
	ZST_EXPORT virtual const size_t payload_size() const override;
	ZST_EXPORT const std::string & sender() const;


private:
	void set_payload(const std::string & payload, std::stringstream & buffer);
	void set_sender(const std::string & sender, std::stringstream & buffer);

	std::string m_sender;
	std::string m_payload;
};
