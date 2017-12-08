#pragma once

#include <string>
#include <vector>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstConstants.h"
#include "entities/ZstEntityBase.h"

#define PLUG_TYPE "plug"

//Forward declarations
class Showtime;
class ZstValue;
class ZstGraphSender;
class ZstComponent;

class ZstPlug : public ZstEntityBase {
public:
	friend class Showtime;
	friend class ZstClient;
    friend class ZstComponent;
    
	//Constructor
	ZST_EXPORT ZstPlug();
	ZST_EXPORT ZstPlug(const char * name, ZstValueType t);
	ZST_EXPORT virtual ~ZstPlug();
    
	ZST_EXPORT virtual void init() override {};
	ZST_EXPORT virtual void register_graph_sender(ZstGraphSender * sender) {};

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
	ZST_EXPORT ZstValue * raw_value();

	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//ZST_EXPORT ZstComponent * owner();

protected:
	ZstValue * m_value;
	ZstPlugDirection m_direction;
};


// --------------------
// Derived plug classes
// --------------------
class ZstInputPlug : public ZstPlug {
public:
	friend class ZstClient;
	friend class Showtime;

	ZST_EXPORT ZstInputPlug(const char * name, ZstValueType t);
	ZST_EXPORT ~ZstInputPlug();
};


class ZstOutputPlug : public ZstPlug {
public:
	ZstOutputPlug(const char * name, ZstValueType t);
	virtual void register_graph_sender(ZstGraphSender * sender);
	ZST_EXPORT void fire();

private:
	ZstGraphSender * m_sender;
};
