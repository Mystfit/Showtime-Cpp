#pragma once

#include <vector>
#include "ZstExports.h"
#include "ZstEntityBase.h"
#include "ZstCallbacks.h"
#include "ZstPlug.h"

#define COMPONENT_TYPE "filter"
class ZstComputeCallback;

class ZstComponent : public ZstEntityBase {
public:
	friend class ZstComputeCallback;
	ZST_EXPORT ZstComponent(const char * entity_type, const char * name);
	ZST_EXPORT ZstComponent(const char * entity_type, const char * name, ZstEntityBase * parent);
	ZST_EXPORT ~ZstComponent();
    
    //Initializer for this component
	ZST_EXPORT virtual void init() override;
    
    //Find a plug in this component by its URI
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI & uri) const;
    
    //Overridable compute function that will process input plug events
	ZST_EXPORT virtual void compute(ZstInputPlug * plug);
    
    //Create and attach a new input plug to this component
	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
    
    //Create and attach a new output plug to this component
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
    
    //Remove a plug from this component
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);

private:
	std::vector<ZstPlug*> m_plugs;
	ZstComputeCallback * m_compute_callback;
};
