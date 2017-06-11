#include <string>
#include <iostream>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstPerformer.h"
#include "ZstStage.h"
#include "ZstEndpoint.h"

ZstStage *stage;
Showtime *performer_a;
Showtime *performer_b;

void test_URI() {
	ZstURI uri_empty = ZstURI();
	//assert(uri_empty.performer().empty() && uri_empty.instrument().empty() && uri_empty.name().empty());

	ZstURI uri_equal1 = ZstURI("perf", "ins", "someplug", ZstURI::Direction::OUT_JACK);
	ZstURI uri_equal2 = ZstURI("perf", "ins", "someplug", ZstURI::Direction::OUT_JACK);
	assert(uri_equal1 == uri_equal2);

	ZstURI uri_notequal = ZstURI("perf", "anotherins", "someplug", ZstURI::Direction::IN_JACK);
	assert(!(uri_equal1 == uri_notequal));
	assert(uri_equal1 != uri_notequal);
}

void test_performer_init() {
	Showtime::endpoint().self_test();
}

//Test stage creation and performer
void test_stage_registration(){
    //Test stage connection
    assert(Showtime::endpoint().ping_stage().count() >= 0);
    
    assert(stage->get_performer_ref_by_name("test_performer_1") != NULL);
    assert(stage->get_performer_ref_by_name("non_existing_performer") == NULL);
}

void test_create_plugs(){
    //Create new plugs
	ZstURI * outURI = ZstURI::create("test_performer_1", "test_instrument", "test_output_plug", ZstURI::Direction::OUT_JACK);
	ZstURI * inURI = ZstURI::create("test_performer_1", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);
	
	//URI ownership taken over by plug
	ZstIntPlug *outputPlug = Showtime::create_plug<ZstIntPlug>(outURI);
	ZstIntPlug *inputPlug = Showtime::create_plug<ZstIntPlug>(inURI);
    
    //Check address equality
    assert(outputPlug->get_URI() == outURI);
    assert(!(outputPlug->get_URI() == inURI));

	//Test creating plug with type functions (blame swig python!)
	ZstIntPlug *typedIntPlug = Showtime::create_int_plug(new ZstURI("test_performer_1", "test_instrument", "test_int_plug", ZstURI::Direction::OUT_JACK));

    //Check stage registered plugs successfully
    ZstPerformerRef *stagePerformerRef = stage->get_performer_ref_by_name("test_performer_1");
    assert(stagePerformerRef->get_plug_by_name(outputPlug->get_URI()->name()) != NULL);
    assert(stagePerformerRef->get_plug_by_name(inputPlug->get_URI()->name()) != NULL);
	assert(stagePerformerRef->get_plug_by_name(typedIntPlug->get_URI()->name()) != NULL);

    //Check local client registered plugs correctly
	ZstPlug *localPlug = Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument")[0];
    //assert(strcmp(localPlug->get_URI().name().c_str(), localPlug->get_URI().name().c_str()) == 0);
    
    std::vector<ZstPlug*> localplugs = Showtime::get_performer_by_name("test_performer_1")->get_plugs();
    assert(localplugs.size() > 1);
    assert(stagePerformerRef->get_plug_by_name(localplugs[0]->get_URI()->name()) != NULL);
    assert(stagePerformerRef->get_plug_by_name(localplugs[1]->get_URI()->name()) != NULL);
    
    //Query stage for remote plugs
    std::vector<ZstURI> plugs = Showtime::endpoint().get_all_plug_addresses();
    assert(plugs.size() > 0);
    plugs = Showtime::endpoint().get_all_plug_addresses("test_performer_1");
    assert(plugs.size() > 0);
    plugs = Showtime::endpoint().get_all_plug_addresses("non_existing_performer");
    assert(plugs.size() == 0);
    plugs = Showtime::endpoint().get_all_plug_addresses("test_performer_1", "test_instrument");
    assert(plugs.size() > 0);

	//Check plug destruction
	std::string outputName = outputPlug->get_URI()->name();
	std::string inputName = inputPlug->get_URI()->name();
	std::string typedName = typedIntPlug->get_URI()->name();

	Showtime::endpoint().destroy_plug(outputPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(outputName) == NULL);
	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").size() == 2);
	
	Showtime::endpoint().destroy_plug(inputPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(inputName) == NULL);
	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").size() == 1);

	Showtime::endpoint().destroy_plug(typedIntPlug);
	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(typedName) == NULL);
	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").empty());
}


class TestIntCallback : public PlugCallback{
public:
    //~TestCallback();
    void run(ZstPlug* plug) override {
        assert(((ZstIntPlug*)plug)->get_value() == 27);
    };
};


void test_connect_plugs() {

	ZstURI * outURI = ZstURI::create("test_performer_1", "test_instrument", "test_output_plug", ZstURI::Direction::OUT_JACK);
	ZstURI * inURI = ZstURI::create("test_performer_1", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);

	//Test plugs connected between performers
	ZstIntPlug *output_int_plug = Showtime::create_plug<ZstIntPlug>(outURI);
	ZstIntPlug *input_int_plug = Showtime::create_plug<ZstIntPlug>(inURI);
    input_int_plug->attach_recv_callback(new TestIntCallback());
	Showtime::connect_plugs(output_int_plug->get_URI(), input_int_plug->get_URI());

    //TODO: First connection, so need to wait for endpoint->stage->endpoint handshake to complete. Futures could help with this?
#ifdef WIN32
    Sleep(500);
#else
    sleep(1);
#endif
	output_int_plug->fire(27);
}


void test_cleanup() {
	//Test object destruction
	delete stage;
	Showtime::endpoint().destroy();
}

int main(int argc,char **argv){
    stage = ZstStage::create_stage();

	std::cout << "Stage created" << std::endl;
#ifdef WIN32
	system("pause");
#endif
    
    Showtime::join("127.0.0.1");
	ZstPerformer * performer_a = Showtime::create_performer("test_performer_1");
	ZstPerformer * performer_b = Showtime::create_performer("test_performer_2");
    
	test_URI();
	test_performer_init();
    test_stage_registration();
    test_create_plugs();
	test_connect_plugs();
	test_cleanup();
	std::cout << "\nShowtime test successful" << std::endl;
#ifdef WIN32
	system("pause");
#endif

	return 0;
}




