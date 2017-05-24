#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <czmq.h>
#include <msgpack.hpp>
#include "ZstExports.h"

enum PlugDir {
	IN_JACK = 0,
	OUT_JACK
};

//Plug output types
struct IntOutput {
	int out;
	MSGPACK_DEFINE(out);
};

struct FloatOutput {
	float out;
	MSGPACK_DEFINE(out);
};

struct StringOutput {
	std::string out;
	MSGPACK_DEFINE(out);
};

struct PlugAddress {
	std::string performer;
	std::string instrument;
	std::string name;
	PlugDir direction;

	inline bool operator==(const PlugAddress& other)
	{
		return (performer == other.performer) &&
			(instrument == other.instrument) &&
			(name == other.name);
	}
	std::string to_s() {
		return performer + "/" + instrument + "/" + name + "/" + std::to_string(direction);
	}
	MSGPACK_DEFINE(performer, instrument, name, direction);
};


class ZstPlug {

public:
	//Constructor
	ZstPlug(std::string name, std::string instrument, std::string performer, PlugDir direction);
	~ZstPlug();

	//Accessors
	ZST_EXPORT std::string get_name();
	ZST_EXPORT std::string get_instrument();
	ZST_EXPORT std::string get_performer();
	ZST_EXPORT PlugDir get_direction();
	ZST_EXPORT PlugAddress get_address();

protected:
	virtual void fire();
	msgpack::sbuffer * m_buffer;
	msgpack::packer<msgpack::sbuffer> * m_packer;

private:
	std::string m_name;
	std::string m_performer;
	std::string m_instrument;
	PlugDir m_direction;
};


enum PlugTypes {
	INT_PLUG = 0,
	INT_ARR_PLUG,
	FLOAT_PLUG,
	FLOAT_ARR_PLUG,
	STRING_PLUG
};


// ----------------------
// Plug types
// ----------------------
class ZstIntPlug : public ZstPlug {
public:
	ZstIntPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(int value);
};
class ZstIntArrayPlug : public ZstPlug { 
public: 
	ZstIntArrayPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(std::vector<int> value);
};
class ZstFloatPlug : public ZstPlug { 
public: 
	ZstFloatPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(float value);
};
class ZstFloatArrayPlug : public ZstPlug { 
public: 
	ZstFloatArrayPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(std::vector<float> value);
};
class ZstStringPlug : public ZstPlug { 
public: 
	ZstStringPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(std::string value);
};

MSGPACK_ADD_ENUM(PlugDir);
