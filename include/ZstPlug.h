#pragma once

#include <string>
#include <vector>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstUtils.hpp"
#include "ZstURI.h"
#include "ZstEvent.h"
#include "Showtime.h"

//Forward declaration
class ZstValue;
class ZstURI;
class PlugCallback;
class Showtime;

class ZstPlug {
public:
	friend class Showtime;
	//Constructor
	ZST_EXPORT ZstPlug(ZstURI * uri, ZstValueType t);
	ZST_EXPORT virtual ~ZstPlug();
	ZST_EXPORT ZstURI * get_URI() const;
	ZST_EXPORT ZstValue * value();

protected:
	ZstValue * m_value;

private:
	ZstURI * m_uri;
};


// ----------------------
// Plug types
// ----------------------
class ZstInputPlugEventCallback;
class ZstInputPlug : public ZstPlug {
public:
	ZstInputPlug(ZstURI * uri, ZstValueType t) : ZstPlug(uri, t) {};
	~ZstInputPlug();
	
	//Plug callbacks
	ZST_EXPORT void attach_recv_callback(ZstInputPlugEventCallback * callback);
	ZST_EXPORT void destroy_recv_callback(ZstInputPlugEventCallback * callback);
	void run_recv_callbacks();

	//Receive a msgpacked value through this plug
	ZST_EXPORT void recv(ZstValue * val);
private:
	std::vector<ZstInputPlugEventCallback*> m_received_data_callbacks;
};

class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(ZstURI * uri, ZstValueType t) : ZstPlug(uri, t) {};
	ZST_EXPORT void fire();
};

class ZstInputPlugEventCallback {
public:
	ZST_EXPORT ZstInputPlugEventCallback();
	virtual ~ZstInputPlugEventCallback() { std::cout << "ZstPlugEventCallback::~ZstPlugEventCallback()" << std::endl; }
	virtual void run(ZstInputPlug * e) { std::cout << "ZstEventCallback::run()" << std::endl; }
};
