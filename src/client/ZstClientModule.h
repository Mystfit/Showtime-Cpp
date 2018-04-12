#pragma once
#include "../core/ZstEventDispatcher.h"

//Forwards
class ZstClient;

class ZstClientModule : public ZstEventDispatcher {
public:
	ZstClientModule(ZstClient * client);
	virtual void init() {};
	virtual void destroy() {};

protected:
	ZstClient * client();
	ZstClientModule();

private:
	ZstClient * m_client;
};
