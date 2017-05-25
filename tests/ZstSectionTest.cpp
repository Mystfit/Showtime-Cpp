#include <string>
#include <iostream>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstPerformer.h"
#include "ZstStage.h"

ZstStage *stage;
Showtime *performer_a;
Showtime *performer_b;

void test_performer_init() {
	Showtime::instance().self_test();
}

//Test stage creation and performer
void test_stage_registration(){
    //Test stage connection
    assert(Showtime::instance().ping_stage().count() >= 0);
    
    assert(stage->get_performer_ref_by_name("test_performer_1") != NULL);
    assert(stage->get_performer_ref_by_name("non_existing_performer") == NULL);
}

void test_create_plugs(){
    //Create new plugs
	ZstIntPlug *outputPlug = Showtime::create_plug<ZstIntPlug>("test_performer_1", "test_output_plug", "test_instrument", PlugDir::OUT_JACK);
	ZstIntPlug *inputPlug = Showtime::create_plug<ZstIntPlug>("test_performer_1", "test_input_plug", "test_instrument", PlugDir::IN_JACK);

    //Check stage registered plugs successfully
    ZstPerformerRef *stagePerformerRef = stage->get_performer_ref_by_name("test_performer_1");
    assert(stagePerformerRef->get_plug_by_name(outputPlug->get_name()) != NULL);
    assert(stagePerformerRef->get_plug_by_name(inputPlug->get_name()) != NULL);
    
    //Check local client registered plugs correctly
	ZstPlug *localPlug = Showtime::get_performer("test_performer_1")->get_instrument_plugs("test_instrument")[0];
    assert(localPlug->get_name() == stagePerformerRef->get_plug_by_name(outputPlug->get_name())->get_address().name);
    
    std::vector<ZstPlug*> localplugs = Showtime::get_performer("test_performer_1")->get_plugs();
    assert(localplugs.size() > 1);
    assert(stagePerformerRef->get_plug_by_name(localplugs[0]->get_name()) != NULL);
    assert(stagePerformerRef->get_plug_by_name(localplugs[1]->get_name()) != NULL);
    
    //Query stage for remote plugs
    std::vector<PlugAddress> plugs = Showtime::instance().get_all_plug_addresses();
    assert(plugs.size() > 0);
    plugs = Showtime::instance().get_all_plug_addresses("test_performer_1");
    assert(plugs.size() > 0);
    plugs = Showtime::instance().get_all_plug_addresses("non_existing_performer");
    assert(plugs.size() == 0);
    plugs = Showtime::instance().get_all_plug_addresses("test_performer_1", "test_instrument");
    assert(plugs.size() > 0);

	//Check plug destruction
	std::string outputName = outputPlug->get_name();
	std::string inputName = inputPlug->get_name();

	Showtime::instance().destroy_plug(outputPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(outputName) == NULL);
	assert(Showtime::get_performer("test_performer_1")->get_instrument_plugs("test_instrument").size() == 1);
	
	Showtime::instance().destroy_plug(inputPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(inputName) == NULL);
	assert(Showtime::get_performer("test_performer_1")->get_instrument_plugs("test_instrument").empty());
}


void test_connect_plugs() {
	//Test plugs connected between performers
	ZstIntPlug *outputPlug = Showtime::create_plug<ZstIntPlug>("test_performer_1", "test_output_plug", "test_instrument", PlugDir::OUT_JACK);
	ZstIntPlug *inputPlug = Showtime::create_plug<ZstIntPlug>("test_performer_2", "test_input_plug", "test_instrument", PlugDir::IN_JACK);

	Showtime::connect_plugs(outputPlug->get_address(), inputPlug->get_address());
    
#ifdef WIN32
    Sleep(500);
#else
    sleep(1);
#endif

	//Send!
	outputPlug->fire(27);
}


void test_cleanup() {
	//Test object destruction
	delete stage;
	Showtime::instance().destroy();
}

int main(int argc,char **argv){
    stage = ZstStage::create_stage();
    
    Showtime::join( "127.0.0.1");
	ZstPerformer * performer_a = Showtime::create_performer("test_performer_1");
	ZstPerformer * performer_b = Showtime::create_performer("test_performer_2");
    
	test_performer_init();
    test_stage_registration();
    test_create_plugs();
	test_connect_plugs();
	test_cleanup();

	std::cout << "Shutting down..." << std::endl;

	std::cout << "Performer test completed" << std::endl;

#ifdef WIN32
	system("pause");
#endif

	return 0;
}




