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
	ZST_EXPORT virtual void connect(const std::string & address);
	ZST_EXPORT virtual void disconnect();
	ZST_EXPORT virtual int bind(const std::string& address);
};

}
