#pragma once

//Forwards
class ZstClient;

class ZstClientModule {
public:
	ZstClientModule(ZstClient * client);
	virtual void init() {};
	virtual void destroy() {};

protected:
	/*ZstClient * client();*/
	ZstClientModule();

private:
	ZstClient * m_client;
};
