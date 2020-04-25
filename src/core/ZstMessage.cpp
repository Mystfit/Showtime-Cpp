#include "ZstMessage.h"
#include "transports/ZstTransportLayerBase.hpp"

namespace showtime 
{
	ZstMessage::ZstMessage() : m_response(false){}

	void ZstMessage::set_has_promise()
	{
		m_response = true;
	}

	bool ZstMessage::has_promise()
	{
		return m_response;
	}
}
