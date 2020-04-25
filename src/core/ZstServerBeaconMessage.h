#pragma once

#include "ZstMessage.h"
#include "schemas/stage_beacon_message_generated.h"

namespace showtime {
	// Forwards
	class ZstServiceDiscoveryTransport;

	class ZstServerBeaconMessage : public ZstMessage {
	public:
		ZST_EXPORT void init(const StageBeaconMessage* buffer, const std::string & address, std::shared_ptr<ZstServiceDiscoveryTransport>& owning_transport);
		ZST_EXPORT const std::string& address() const;
		ZST_EXPORT const StageBeaconMessage* buffer() const;

		ZST_EXPORT virtual void reset() override;
		ZST_EXPORT virtual ZstMsgID id() const override;
		ZST_EXPORT virtual const uuid& origin_endpoint_UUID() const override;
		ZST_EXPORT virtual std::shared_ptr<ZstTransportLayerBase> owning_transport() const override;

	private:
		std::string m_address;
		const StageBeaconMessage* m_buffer;
		std::weak_ptr<ZstServiceDiscoveryTransport> m_owning_transport;
	};
}
