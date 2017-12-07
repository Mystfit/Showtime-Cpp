#pragma once

#include <vector>
#include "ZstExports.h"
#include "ZstEntityBase.h"
#include "ZstPlug.h"

#define COMPONENT_TYPE "component"

class ZstComponent : public ZstEntityBase {
public:
	friend class ZstClient;
	ZST_EXPORT ZstComponent();
	ZST_EXPORT ZstComponent(const char * component_type);
	ZST_EXPORT ZstComponent(const char * component_type, const char * path);
	ZST_EXPORT ~ZstComponent();

	ZST_EXPORT virtual void init() override {};

	//Register graph sender for output plugs
	ZST_EXPORT virtual void register_graph_sender(ZstGraphSender * sender);
    
    //External factory function
	ZST_EXPORT virtual void create(const char * name, ZstEntityBase* parent) {};
    
    //Find a plug in this component by its URI
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI & uri) const;
    
    //Overridable compute function that will process input plug events
	ZST_EXPORT virtual void compute(ZstInputPlug * plug) {};
        
    //Create and attach a new input plug to this component
	ZST_EXPORT ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
    
    //Create and attach a new output plug to this component
	ZST_EXPORT ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
    
    //Remove a plug from this component
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);

	//Serialisation
	virtual void write(std::stringstream & buffer) override;
	virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//Speicifc component type
	const char * component_type() const;

protected:
	ZST_EXPORT void set_component_type(const char * component_type);
	
	//Transfer plug ownership to this component
	ZST_EXPORT int add_plug(ZstPlug * plug);

private:
	std::vector<ZstPlug*> m_plugs;
	char * m_component_type;
};
