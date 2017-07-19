#pragma once

#include <string>
#include <vector>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstUtils.hpp"
#include "ZstURI.h"
#include "ZstEvent.h"
#include "ZstCallbackQueue.h"
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
// Plug callbacks
// ----------------------

class ZstPlugEventCallback {
public:
	ZST_EXPORT virtual ~ZstPlugEventCallback() { std::cout << "Destroying plug event callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstURI plug) { std::cout << "Running plug event callback" << std::endl; }
};


class ZstInputPlug;
class ZstInputPlugEventCallback {
public:
	ZST_EXPORT virtual ~ZstInputPlugEventCallback() { std::cout << "Destroying input callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstInputPlug * e) { std::cout << "Input callback running" << std::endl; }
};


class ZstInputPlug : public ZstPlug {
public:
	ZST_EXPORT ZstInputPlug(ZstURI * uri, ZstValueType t);
	ZST_EXPORT ~ZstInputPlug();

	//Receive a msgpacked value through this plug
	ZST_EXPORT void recv(ZstValue * val);
	ZST_EXPORT ZstCallbackQueue<ZstInputPlugEventCallback, ZstInputPlug*> * input_events();
private:
	ZstCallbackQueue<ZstInputPlugEventCallback, ZstInputPlug*> * m_input_fired_manager;
};


class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(ZstURI * uri, ZstValueType t) : ZstPlug(uri, t) {};
	ZST_EXPORT void fire();
};

