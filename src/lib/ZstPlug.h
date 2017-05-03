#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include "ZstExports.h"

class ZstPlug {

public:
    enum PlugMode {
        READABLE = 0,
        WRITEABLE
    };

    //Constructor
    ZstPlug(std::string name, PlugMode mode);
    
    //Accessors
    ZST_EXPORT std::string get_name();
    ZST_EXPORT PlugMode get_mode();

private:
    std::string m_name;
    PlugMode m_plug_mode;
    
    //Inputs
    std::vector<std::string> m_args;
    
    //Outputs
    std::string m_output;
    bool m_output_ready = false;
};

