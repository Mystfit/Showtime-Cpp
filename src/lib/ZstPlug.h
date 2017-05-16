#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include "ZstExports.h"

class ZstPlug {

public:
    enum PlugDirection {
        INPUT = 0,
        OUTPUT
    };

    //Constructor
    ZstPlug(std::string name, std::string instrument, PlugDirection mode);
    
    //Accessors
    ZST_EXPORT std::string get_name();
    ZST_EXPORT PlugDirection get_mode();

private:
    std::string m_name;
    std::string m_instrument;
    PlugDirection m_plug_mode;
    
    //Inputs
    std::vector<std::string> m_args;
    
    //Outputs
    std::string m_output;
    bool m_output_ready = false;
};
