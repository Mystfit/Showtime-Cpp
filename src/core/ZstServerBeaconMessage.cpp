#include "ZstServerBeaconMessage.h"
#include <boost/uuid/nil_generator.hpp>
#include "transports/ZstServiceDiscoveryTransport.h"

namespace showtime {
	void ZstServerBeaconMessage::init(const StageBeaconMessage* buffer, const std::string& address, std::shared_ptr<ZstServiceDiscoveryTransport>& owning_transport)
	{
		m_buffer = buffer;
		m_address = address;
		m_owning_transport = owning_transport;
	}

	const StageBeaconMessage* ZstServerBeaconMessage::buffer() const {
		return m_buffer;
	}

	void ZstServerBeaconMessage::reset()
	{
		m_address = "";
		m_buffer = NULL;
		m_owning_transport.reset();
	}

	ZstMsgID ZstServerBeaconMessage::id() const
	{
		return nil_generator()();
	}

	const uuid& ZstServerBeaconMessage::origin_endpoint_UUID() const
	{
		return boost::uuids::nil_uuid();
	}

	std::shared_ptr<ZstTransportLayerBase> ZstServerBeaconMessage::owning_transport() const
	{
		return std::static_pointer_cast<ZstTransportLayerBase>(m_owning_transport.lock());
	}

	const std::string& showtime::ZstServerBeaconMessage::address() const
	{
		return m_address;
	}
}
