#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <czmq.h>
#include <msgpack.hpp>
#include "ZstExports.h"
#include "ZstUtils.hpp"

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
    
    bool operator < (const PlugAddress& a) const {   return to_s() < a.to_s();  }

	std::string to_s() const {
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
    
    static PlugAddress address_from_str(std::string s){
        PlugAddress address;
        std::vector<std::string> tokens;
        Utils::str_split(s, tokens, "/");
        address.performer = tokens[0];
        address.instrument = tokens[1];
        address.name = tokens[2];
        address.direction = (PlugDir)std::atoi(tokens[3].c_str());
        return address;
    }
    virtual void recv(msgpack::object obj) = 0;


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
    void recv(msgpack::object object) override;
    int get_value();
private:
    int m_last_value;
    int m_value;
};
class ZstIntListPlug : public ZstPlug {
public: 
	ZstIntListPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(std::vector<int> value);
    void recv(msgpack::object object) override;
    std::vector<int> get_value();
private:
    std::vector<int> m_last_value;
    std::vector<int> m_value;
};
class ZstFloatPlug : public ZstPlug { 
public: 
	ZstFloatPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(float value);
    void recv(msgpack::object object) override;
    float get_value();
private:
    float m_last_value;
    float m_value;
};
class ZstFloatListPlug : public ZstPlug {
public: 
	ZstFloatListPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(std::vector<float> value);
    void recv(msgpack::object object) override;
    std::vector<float> get_value();
private:
    std::vector<float> m_last_value;
    std::vector<float> m_value;
};
class ZstStringPlug : public ZstPlug { 
public: 
	ZstStringPlug(std::string name, std::string instrument, std::string performer, PlugDir direction) : ZstPlug(name, instrument, performer, direction) {};
	ZST_EXPORT void fire(std::string value);
    void recv(msgpack::object object) override;
    std::string get_value();
private:
    std::string m_last_value;
    std::string m_value;
};

MSGPACK_ADD_ENUM(PlugDir);
