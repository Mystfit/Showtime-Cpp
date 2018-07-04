#pragma once

#include <ZstExports.h>
#include <adaptors/ZstEventAdaptor.hpp>

//Forwards
class ZstOutputPlug;

class ZstOutputPlugAdaptor : public ZstEventAdaptor {
public:
	ZST_EXPORT virtual void on_plug_fire(ZstOutputPlug * plug);
};
