#pragma once

#include <vector>
#include "ZstExports.h"
#include "ZstEntityBase.h"
#include "ZstCallbacks.h"
#include "ZstPlug.h"

#define COMPONENT_TYPE "filter"

class ZstComponent : public ZstEntityBase {
public:
	ZST_EXPORT ZstComponent(const char * entity_type, const char * path);
	ZST_EXPORT ~ZstComponent();
    
    //Initializer for this component
	ZST_EXPORT virtual void init() override;
    
    ZST_EXPORT virtual void destroy() override;
    
    //External factory function
    ZST_EXPORT virtual void create(const char * name, ZstEntityBase* parent) = 0;
    
    ZST_EXPORT bool is_registered();
    
    //Sync this entity with the stage
    ZST_EXPORT virtual void activate();
    
    //Find a plug in this component by its URI
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI & uri) const;
    
    //Overridable compute function that will process input plug events
	ZST_EXPORT virtual void compute(ZstInputPlug * plug) = 0;
        
    //Create and attach a new input plug to this component
	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
    
    //Create and attach a new output plug to this component
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
    
    //Remove a plug from this component
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);
    

private:
	std::vector<ZstPlug*> m_plugs;
    bool m_is_registered;
};
