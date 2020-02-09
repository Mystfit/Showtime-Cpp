#pragma once

#include "ZstMessage.h"
#include "schemas/stage_beacon_message_generated.h"

namespace showtime {
	class ZstServerBeaconMessage : ZstMessage {
	public:
		ZST_EXPORT void init(const StageBeaconMessage* buffer, const std::string & address);
		ZST_EXPORT const std::string& address() const;
		ZST_EXPORT const StageBeaconMessage* buffer() const;

		ZST_EXPORT virtual void reset() override;
		ZST_EXPORT virtual ZstMsgID id() const override;
		ZST_EXPORT virtual const uuid& endpoint_UUID() const override;

	private:
		std::string m_address;
		const StageBeaconMessage* m_buffer;
	};
}
