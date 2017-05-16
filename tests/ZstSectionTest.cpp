#include <string>
#include <iostream>
#include "ZstPerformance.h"
#include "ZstPlug.h"
#include "ZstStage.h"

ZstStage *stage;
ZstPerformance *performer_a;
ZstPerformance *performer_b;

//Test stage creation and performer
void test_stage_registration(){
    //Test stage connection
    assert(performer_a->ping_stage().count() >= 0);
    
    //Test performer registration
    performer_a->register_to_stage();
    
    ZstPerformerRef testperformer = stage->get_performer_ref("test_performer_1");
    assert(testperformer.name == "test_performer_1");
    
    bool missingperformerException = false;
    try{
        stage->get_performer_ref("non_existing_performer");
    } catch(std::out_of_range&){
        missingperformerException = true;
    }
    assert(missingperformerException);
}

void test_create_plugs(){
    //Create new plugs
    ZstPlug *outputPlug = performer_a->create_plug("test_output_plug", "test_instrument", ZstPlug::PlugDirection::OUTPUT);
    ZstPlug *inputPlug = performer_a->create_plug("test_input_plug", "test_instrument", ZstPlug::PlugDirection::INPUT);

    //Check stage registered plugs successfully
    ZstPerformerRef stagePerformerRef = stage->get_performer_ref(performer_a->get_performer_name());
    assert(stagePerformerRef.plugs.size() > 0);
    assert(stagePerformerRef.plugs[0].name == outputPlug->get_name());
    assert(stagePerformerRef.plugs.size() > 1);
    assert(stagePerformerRef.plugs[1].name == inputPlug->get_name());
    
    //Check local client registered plugs correctly
    assert(performer_a->get_instrument_plugs("test_instrument")[0]->get_name() == stagePerformerRef.plugs[0].name);
    
    std::vector<ZstPlug*> localplugs = performer_a->get_all_plugs();
    assert(localplugs.size() > 1);
    assert(localplugs[0]->get_name() == stagePerformerRef.plugs[0].name);
    assert(localplugs[1]->get_name() == stagePerformerRef.plugs[1].name);
    
    //Query stage for remote plugs
    std::vector<ZstPlugAddress> plugs = performer_a->get_plug_addresses();
    assert(plugs.size() > 0);
    plugs = performer_a->get_plug_addresses("test_performer_1");
    assert(plugs.size() > 0);
    plugs = performer_a->get_plug_addresses("non_existing_performer");
    assert(plugs.size() == 0);
    plugs = performer_a->get_plug_addresses("test_performer_1", "test_instrument");
    assert(plugs.size() > 0);
}

void test_query_stage(){
    
}

int main(int argc,char **argv){
    stage = ZstStage::create_stage();
    
	performer_a = ZstPerformance::create_performer("test_performer_1");
    
    test_stage_registration();
    test_create_plugs();
    test_query_stage();
    
	return 0;
}




