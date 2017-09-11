#pragma once

#include <string>
#include <vector>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstEvent.h"
#include "ZstCallbackQueue.h"
#include "ZstCallbacks.h"
#include "ZstConstants.h"

//Forward declarations
class ZstURI;
class PlugCallback;
class Showtime;
class ZstComponent;
class ZstValue;

class ZstPlug {
public:
	friend class Showtime;
	friend class ZstEndpoint;
	//Constructor
	ZST_EXPORT ZstPlug(ZstComponent * owner, const char * name, ZstValueType t);
	ZST_EXPORT virtual ~ZstPlug();

	ZST_EXPORT const ZstURI & get_URI() const;
	ZST_EXPORT const ZstEntityBase* owner() const;
	ZST_EXPORT bool is_destroyed();

	//Value interface
	ZST_EXPORT void clear();
	ZST_EXPORT void append_int(int value);
	ZST_EXPORT void append_float(float value);
	ZST_EXPORT void append_char(const char * value);

	ZST_EXPORT const size_t size() const;
	ZST_EXPORT const int int_at(const size_t position) const;
	ZST_EXPORT const float float_at(const size_t position) const;
	ZST_EXPORT void char_at(char * buf, const size_t position) const;
	ZST_EXPORT const size_t size_at(const size_t position) const;

protected:
	ZstValue * m_value;

private:
	ZstComponent * m_owner;
	ZstURI m_uri;
	bool m_is_destroyed;
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
	friend class ZstEndpoint;
	friend class Showtime;

	ZST_EXPORT ZstInputPlug(ZstComponent * owner, const char * name, ZstValueType t);
	ZST_EXPORT ~ZstInputPlug();
	ZST_EXPORT void attach_receive_callback(ZstPlugDataEventCallback * callback);
	ZST_EXPORT void remove_receive_callback(ZstPlugDataEventCallback * callback);
protected:

private:
	//Receive a msgpacked value through this plug
	ZST_EXPORT void recv(ZstValue * val);
	ZstCallbackQueue<ZstPlugDataEventCallback, ZstInputPlug*> * m_input_fired_manager;
};


class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(ZstComponent * owner, const char * name, ZstValueType t) : ZstPlug(owner, name, t) {};
	ZST_EXPORT void fire();
};
