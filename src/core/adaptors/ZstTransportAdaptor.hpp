#pragma once

#include "showtime/schemas/messaging/stage_message_generated.h"

#include <showtime/ZstExports.h>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {

//Forwards
class ZstMessage;

class ZST_CLASS_EXPORTED ZstTransportAdaptor : 
	public ZstEventAdaptor<ZstTransportAdaptor>
{
public:
	//Incoming events
	ZST_EXPORT virtual void connect(const std::string & address);
	ZST_EXPORT virtual void disconnect();
	ZST_EXPORT virtual int bind(const std::string& address);
};

}
