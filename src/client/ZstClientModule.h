#pragma once
#include "ZstClient.h"
#include "../core/ZstEventDispatcher.h"

class ZstClientModule : public ZstEventDispatcher {
public:
	ZstClientModule(ZstClient * client);
	virtual void init() {};
	virtual void destroy() {};

protected:
	ZstClient * client();

private:
	ZstClient * m_client;
	ZstClientModule();
};
