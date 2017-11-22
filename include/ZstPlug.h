#pragma once

#include <string>
#include <vector>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstCallbackQueue.h"
#include "ZstCallbacks.h"
#include "ZstConstants.h"
#include "entities/ZstEntityBase.h"

//Forward declarations
class ZstURI;
class Showtime;
class ZstValue;
class ZstComponent;


enum PlugDirection {
    NONE = 0,
    IN_JACK,
    OUT_JACK
};


class ZstPlug : public ZstEntityBase {
public:
	friend class Showtime;
	friend class ZstEndpoint;
    friend class ZstComponent;
    
	//Constructor
	ZST_EXPORT ZstPlug(ZstComponent * owner, const char * name, ZstValueType t);
	ZST_EXPORT virtual ~ZstPlug();
    
    virtual void init() override {};

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
    PlugDirection m_direction;
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
protected:

private:
	//Receive a msgpacked value through this plug
	ZST_EXPORT void recv(const ZstValue & val);
};


class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(ZstComponent * owner, const char * name, ZstValueType t);
	ZST_EXPORT void fire();
};
