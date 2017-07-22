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
#include "ZstCallbacks.h"
#include "ZstValue.h"
#include "Showtime.h"
#include "entities\ZstEntityBase.h"

//Forward declarations
class ZstURI;
class PlugCallback;
class Showtime;

class ZstPlug {
public:
	friend class Showtime;
	//Constructor
	ZST_EXPORT ZstPlug(ZstEntityBase * owner, const char * name, ZstValueType t);
	ZST_EXPORT virtual ~ZstPlug();
	ZST_EXPORT ZstURI get_URI() const;
	ZST_EXPORT ZstValue * value();

protected:
	ZstValue * m_value;

private:
	ZstEntityBase * m_owner;
	ZstURI m_uri;
};


enum PlugDirection {
    NONE = 0,
    IN_JACK,
    OUT_JACK
};


// --------------------
// Derived plug classes
// --------------------

class ZstInputPlug : public ZstPlug {
public:
	ZST_EXPORT ZstInputPlug(ZstEntityBase * owner, const char * name, ZstValueType t);
	ZST_EXPORT ~ZstInputPlug();

	//Receive a msgpacked value through this plug
	ZST_EXPORT void recv(ZstValue * val);
	ZST_EXPORT ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*> * input_events();
private:
	ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*> * m_input_fired_manager;
};


class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(ZstEntityBase * owner, const char * name, ZstValueType t) : ZstPlug(owner, name, t) {};
	ZST_EXPORT void fire();
};

