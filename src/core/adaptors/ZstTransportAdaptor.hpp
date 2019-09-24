#pragma once

#include <schemas/stage_message_generated.h>

#include "ZstExports.h"
#include "adaptors/ZstEventAdaptor.hpp"
#include "../transports/ZstTransportLayerBase.hpp"

namespace showtime {

//Forwards
class ZstMessage;

class ZST_CLASS_EXPORTED ZstTransportAdaptor : 
	public ZstEventAdaptor
{
public:
	//Incoming events
	ZST_EXPORT virtual void connect(const std::string & address) = 0;
	ZST_EXPORT virtual void disconnect() = 0;
	ZST_EXPORT virtual void bind(const std::string& address) = 0;
};

}
