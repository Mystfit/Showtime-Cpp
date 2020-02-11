#include "ZstServerBeaconMessage.h"
#include <boost/uuid/nil_generator.hpp>

namespace showtime {
	void ZstServerBeaconMessage::init(const StageBeaconMessage* buffer, const std::string& address)
	{
		m_buffer = buffer;
		m_address = address;
	}

	const StageBeaconMessage* ZstServerBeaconMessage::buffer() const {
		return m_buffer;
	}

	void ZstServerBeaconMessage::reset()
	{
		m_address = "";
		m_buffer = NULL;
	}

	ZstMsgID ZstServerBeaconMessage::id() const
	{
		return nil_generator()();
	}

	const uuid& ZstServerBeaconMessage::endpoint_UUID() const
	{
		return nil_generator()();
	}

	const std::string& showtime::ZstServerBeaconMessage::address() const
	{
		return m_address;
	}
}
