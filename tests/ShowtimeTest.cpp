#include <string>
#include <vector>
#include <tuple>
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
	assert(uri_empty.performer().empty() && uri_empty.instrument().empty() && uri_empty.name().empty());

	ZstURI uri_equal1 = ZstURI("perf", "ins", "someplug", ZstURI::Direction::OUT_JACK);
	ZstURI uri_equal2 = ZstURI("perf", "ins", "someplug", ZstURI::Direction::OUT_JACK);
	assert(uri_equal1 == uri_equal2);

	ZstURI uri_notequal = ZstURI("perf", "anotherins", "someplug", ZstURI::Direction::IN_JACK);
	assert(!(uri_equal1 == uri_notequal));
	assert(uri_equal1 != uri_notequal);
    
    assert(uri_equal1.to_str() == "perf/ins/someplug?d=1");

    uri_equal1 = ZstURI("ableton_perf", "", "someplug", ZstURI::Direction::OUT_JACK);
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
	ZstIntPlug *outputPlug = Showtime::create_int_plug(outURI);
	std::cout << Showtime::event_queue_size() << std::endl;

	ZstIntPlug *inputPlug = Showtime::create_int_plug(inURI);


    
    //Check address equality
    assert(outputPlug->get_URI() == outURI);
    assert(!(outputPlug->get_URI() == inURI));

	//Test creating plug with type functions (blame swig python!)
	ZstIntPlug *typedIntPlug = Showtime::create_int_plug(new ZstURI("test_performer_1", "test_instrument", "test_int_plug", ZstURI::Direction::OUT_JACK));

    //Check stage registered plugs successfully
    ZstPerformerRef *stagePerformerRef = stage->get_performer_ref_by_name("test_performer_1");
    assert(stagePerformerRef->get_plug_by_URI(outputPlug->get_URI()->to_str()) != NULL);
    assert(stagePerformerRef->get_plug_by_URI(inputPlug->get_URI()->to_str()) != NULL);
	assert(stagePerformerRef->get_plug_by_URI(typedIntPlug->get_URI()->to_str()) != NULL);

    std::cout << "!!!FIXME: get_performer_by_URI in test program" << std::endl;
//  Check local client registered plugs correctly
//  ZstPlug *localPlug = Showtime::get_performer_by_URI("test_performer_1")->get_instrument_plugs("test_instrument")[0];
//  assert(strcmp(localPlug->get_URI()->name().c_str(), localPlug->get_URI()->name().c_str()) == 0);
    
//  std::vector<ZstPlug*> localplugs = Showtime::get_performer_by_name("test_performer_1")->get_plugs();
//  assert(localplugs.size() > 1);
//  assert(stagePerformerRef->get_plug_by_name(localplugs[0]->get_URI()->name()) != NULL);
//  assert(stagePerformerRef->get_plug_by_name(localplugs[1]->get_URI()->name()) != NULL);
    

	//Check plug destruction
	std::string outputName = outputPlug->get_URI()->name();
	std::string inputName = inputPlug->get_URI()->name();
	std::string typedName = typedIntPlug->get_URI()->name();

    std:: cout << "FIXME: Getting plug by name" << std::endl;
	Showtime::endpoint().destroy_plug(outputPlug);
//	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(outputName) == NULL);
//	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").size() == 2);
	
	Showtime::endpoint().destroy_plug(inputPlug);
//	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(inputName) == NULL);
//	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").size() == 1);

	Showtime::endpoint().destroy_plug(typedIntPlug);
//	assert(stage->get_performer_ref_by_name("test_performer_1")->get_plug_by_name(typedName) == NULL);
//	assert(Showtime::get_performer_by_name("test_performer_1")->get_instrument_plugs("test_instrument").empty());
}


class TestIntCallback : public PlugCallback{
public:
	bool fired = false;
    //~TestCallback();
    void run(ZstPlug* plug) override {
		assert(plug);
		fired = true;
    };
};


void test_connect_plugs() {

	ZstURI * outURI = ZstURI::create("test_performer_1", "test_instrument", "test_output_plug", ZstURI::Direction::OUT_JACK);
	ZstURI * inURI = ZstURI::create("test_performer_1", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);
	ZstURI * badURI = ZstURI::create("fake_performer", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);
    
    // ZstURI * ableton_uri = ZstURI::create("test_performer_1", "A-Reverb/Reverb/PreDelay", "out", ZstURI::Direction::OUT_JACK);
    // ZstIntPlug *ableton_plug = Showtime::create_int_plug(ableton_uri);
    //ableton_perf/A-Reverb/Reverb/PreDelay/out?d=1

	//Test plugs connected between performers
	ZstIntPlug *output_int_plug = Showtime::create_int_plug(outURI);
	ZstIntPlug *input_int_plug = Showtime::create_int_plug(inURI);
    input_int_plug->attach_recv_callback(new TestIntCallback());
	Showtime::connect_plugs(output_int_plug->get_URI(), input_int_plug->get_URI());

	//Test connecting missing URI objects
	bool exceptionThrown = false;
	try{
		Showtime::connect_plugs(output_int_plug->get_URI(), badURI);
	}
	catch (std::runtime_error&){
		exceptionThrown = true;
	}
	assert(exceptionThrown);


    //TODO: First connection, so need to wait for endpoint->stage->endpoint handshake to complete. Futures could help with this?
#ifdef WIN32
    Sleep(500);
#else
    sleep(1);
#endif
	int num_fires = 5;
	for (int i = 0; i < num_fires; ++i) {
		output_int_plug->fire(i);
	}

#ifdef WIN32
	Sleep(500);
#else
	sleep(1);
#endif
	//Test event queue. This is thread safe so we can pop each event off at our leisure
	for (int i = num_fires; i > 0; --i) {
		assert(Showtime::event_queue_size() == i);
		ZstEvent e = Showtime::pop_event();
		assert(e.get_update_type() == ZstEvent::EventType::PLUG_HIT);
		assert(e.get_first().to_str() == input_int_plug->get_URI()->to_str());
		assert(Showtime::event_queue_size() == i-1);
	}

	std::cout << "Queue test successful" << std::endl;
	Showtime::destroy_plug(output_int_plug);
	Showtime::destroy_plug(input_int_plug);
}


void test_cleanup() {
	//Test object destruction
	delete stage;
	Showtime::destroy();
}

int main(int argc,char **argv){
    stage = ZstStage::create_stage();
	std::cout << "Stage created" << std::endl;

	Showtime::init();
    Showtime::join("127.0.0.1");

#ifdef WIN32
	Sleep(500);
#else
	sleep(1);
#endif

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
