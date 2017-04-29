#pragma once  

#include <string>
#include <memory>
#include "ZstExports.h"
#include "ZstPlug.h"

namespace Showtime {

    using namespace std;

    class ZstInstrument {
    public:
        ZstInstrument(string name);
        ~ZstInstrument();

        //Plug factory
        ZST_EXPORT ZstPlug* create_plug(string name, ZstPlug::PlugMode plugMode);

        //Accessors
        ZST_EXPORT string get_name();

        ZST_EXPORT vector<ZstPlug*> get_outputs();
        ZST_EXPORT vector<ZstPlug*> get_inputs();


    private:
        string m_name;
        vector<ZstPlug*> m_outputs;
        vector<ZstPlug*> m_inputs;
    };
}

