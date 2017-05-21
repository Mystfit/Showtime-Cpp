#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"

enum PlugDirection {
    INPUT = 0,
    OUTPUT
};

//Plug output types
struct IntOutput{
    int out;
    MSGPACK_DEFINE(out);
};

struct FloatOutput{
    float out;
    MSGPACK_DEFINE(out);
};

struct StringOutput{
    std::string out;
    MSGPACK_DEFINE(out);
};

/*
struct ListIntOutput{
    int out[];
    MSGPACK_DEFINE_ARRAY(out);
};

struct ListFloatOutput{
    float out[];
    MSGPACK_DEFINE_ARRAY(out);
};
 */

struct PlugAddress {
    std::string performer;
    std::string instrument;
    std::string name;
    PlugDirection direction;
    
    inline bool operator==(const PlugAddress& other)
    {
        return (performer == other.performer) &&
        (instrument == other.instrument) &&
        (name == other.name);
    }
    std::string to_s(){
        return performer+"/"+instrument+"/"+name+"/"+std::to_string(direction);
    }
    MSGPACK_DEFINE(performer, instrument, name, direction);
};


class ZstPlug {

public:
    //Constructor
    ZstPlug(std::string name, std::string instrument, std::string performer, PlugDirection direction);
	~ZstPlug();
    
    //Accessors
    ZST_EXPORT std::string get_name();
	ZST_EXPORT std::string get_instrument();
    ZST_EXPORT std::string get_performer();
    ZST_EXPORT PlugDirection get_direction();
	ZST_EXPORT PlugAddress get_address();
    
    ZST_EXPORT void fire(const void * value);

private:
    std::string m_name;
    std::string m_performer;
    std::string m_instrument;
    PlugDirection m_direction;
    
    //Inputs
    std::vector<std::string> m_args;
    
    //Outputs
    std::string m_output;
    bool m_output_ready = false;
};

MSGPACK_ADD_ENUM(PlugDirection);

