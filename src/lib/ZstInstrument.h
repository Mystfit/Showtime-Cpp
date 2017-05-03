#pragma once  

#include <string>
#include <memory>
#include "ZstExports.h"
#include "ZstPlug.h"

class ZstInstrument {
public:
    ZstInstrument(std::string name);
    ~ZstInstrument();

    //Plug factory
    ZST_EXPORT ZstPlug* create_plug(std::string name, ZstPlug::PlugMode plugMode);

    //Accessors
    ZST_EXPORT std::string get_name();

    ZST_EXPORT std::vector<ZstPlug*> get_outputs();
    ZST_EXPORT std::vector<ZstPlug*> get_inputs();


private:
    std::string m_name;
    std::vector<ZstPlug*> m_outputs;
    std::vector<ZstPlug*> m_inputs;
};


