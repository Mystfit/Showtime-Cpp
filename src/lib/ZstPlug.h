#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <msgpack.hpp>
#include "ZstExports.h"

class ZstPlug {

public:
    enum Direction {
        INPUT = 0,
        OUTPUT
    };

	struct Address {
		std::string performer;
		std::string instrument;
		std::string name;
		ZstPlug::Direction direction;
		
		inline bool operator==(const Address& other)
		{
			return (performer == other.performer) &&
				(instrument == other.instrument) &&
				(name == other.name) &&
				(direction == other.direction);
		}
		MSGPACK_DEFINE(performer, instrument, name, direction);
	};

    //Constructor
    ZstPlug(std::string name, std::string instrument, std::string performer, Direction direction);
	~ZstPlug();
    
    //Accessors
    ZST_EXPORT std::string get_name();
	ZST_EXPORT std::string get_instrument();
	ZST_EXPORT std::string get_performer();
    ZST_EXPORT Direction get_direction();
	ZST_EXPORT Address get_address();

private:
    std::string m_name;
    std::string m_instrument;
	std::string m_performer;
    Direction m_direction;
    
    //Inputs
    std::vector<std::string> m_args;
    
    //Outputs
    std::string m_output;
    bool m_output_ready = false;
};

MSGPACK_ADD_ENUM(ZstPlug::Direction);

