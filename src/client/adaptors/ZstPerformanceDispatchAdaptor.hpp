


#pragma once

#include <ZstExports.h>
#include <ZstConstants.h>
#include <entities/ZstPlug.h>
#include <adaptors/ZstEventAdaptor.hpp>
#include "../core/ZstMessage.h"


class ZstPerformanceDispatchAdaptor : public ZstEventAdaptor {
public:
	ZST_CLIENT_EXPORT virtual void send_to_performance(ZstOutputPlug * plug);
	ZST_CLIENT_EXPORT virtual void on_receive_from_performance(ZstMessage * msg);
};
