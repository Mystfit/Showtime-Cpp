#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <type_traits>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstPerformer.h"
#include "ZstStage.h"
#include "ZstEndpoint.h"

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH sleep(1);
#endif

ZstStage *stage;
ZstPerformer *performer_a;

void test_URI() {

	//Verify standard layout
	assert(std::is_standard_layout<ZstURI>());
	assert(std::is_standard_layout<ZstCable>());

	ZstURI * uri_empty = ZstURI::create_empty();
	ZstURI * uri_equal1 = ZstURI::create("perf", "ins", "someplug", ZstURI::Direction::OUT_JACK);
	ZstURI * uri_notequal = ZstURI::create("perf", "anotherins", "someplug", ZstURI::Direction::IN_JACK);

	assert(uri_empty->is_empty());
	assert(*uri_equal1 == *uri_equal1);
	assert(*uri_equal1 != *uri_notequal);
    assert(strcmp(uri_equal1->to_char() , "perf/ins/someplug?d=2") == 0);

	//Test ZstCable equality against ZstURI instances
	ZstCable test_cable_eq1 = ZstCable(*uri_equal1, *uri_empty);
	ZstCable test_cable_neq1 = ZstCable(*uri_equal1, *uri_notequal);

	assert(test_cable_eq1 == test_cable_eq1);
	assert(test_cable_eq1 != test_cable_neq1);
	assert(test_cable_eq1.is_attached(*uri_equal1));
	assert(test_cable_eq1.is_attached(*uri_empty));
	assert(!(test_cable_neq1.is_attached(*uri_empty)));

	ZstURI::destroy(uri_notequal);
	ZstURI::destroy(uri_equal1);
	ZstURI::destroy(uri_empty);
}

void test_performer_init() {
	Showtime::endpoint().self_test();
	performer_a = Showtime::create_performer("test_performer_1");
	assert(performer_a);
	ZstEvent e = Showtime::pop_event();
	assert(strcmp(e.get_first().to_char(), "test_performer_1//?d=0") == 0);
}

//Test stage creation and performer
void test_stage_registration(){
    //Test stage connection
    assert(Showtime::endpoint().ping_stage() >= 0);
    assert(stage->get_performer_ref_by_name("test_performer_1") != NULL);
    assert(stage->get_performer_ref_by_name("non_existing_performer") == NULL);
}

//Callback definitions and trackers
int stageEventHits = 0;
int intPlugCallbacks = 0;
int connectionCallbacks = 0;

class TestStageEventCallback : public ZstEventCallback {
public:
	void run(ZstEvent e) override {
		stageEventHits++;
	};
};

class TestIntValueCallback : public ZstEventCallback {
public:
	void run(ZstEvent e) override {
		intPlugCallbacks++;
	};
};

class TestConnectionCallback : public ZstEventCallback {
public:
	void run(ZstEvent e) override {
		connectionCallbacks++;
	};
};



void test_create_plugs(){

	//Attach stage event callbacks first so we get status updates from stage on plug creation
	Showtime::attach_stage_event_callback(new TestStageEventCallback());

    //Create new plugs
	ZstURI * outURI = ZstURI::create("test_performer_1", "test_instrument", "test_output_plug", ZstURI::Direction::OUT_JACK);
	ZstURI * inURI = ZstURI::create("test_performer_1", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);
	
	//Stack ZstURI copies
	ZstURI outURI_stack = ZstURI(*outURI);
	ZstURI inURI_stack = ZstURI(*inURI);

	//URI ownership taken over by plug
	ZstIntPlug *outputPlug = Showtime::create_int_plug(outURI);
	ZstIntPlug *inputPlug = Showtime::create_int_plug(inURI);
    
    //Check address equality
    assert(outputPlug->get_URI() == outURI);
    assert(!(outputPlug->get_URI() == inURI));

	//Test creating plug with type functions (blame swig python!)
	ZstIntPlug *typedIntPlug = Showtime::create_int_plug(ZstURI::create("test_performer_1", "test_instrument", "test_int_plug", ZstURI::Direction::OUT_JACK));
	ZstURI typedIntPlug_stack = ZstURI(*typedIntPlug->get_URI());

	//Check that our plug creation callbacks fired successfully
	TAKE_A_BREATH
	assert(Showtime::event_queue_size() > 0);
	Showtime::poll_once();
	assert(Showtime::event_queue_size() == 0);
	assert(stageEventHits == 3);
	stageEventHits = 0;

    //Check stage registered plugs successfully
    ZstPerformerRef *stagePerformerRef = stage->get_performer_ref_by_name("test_performer_1");
    assert(stagePerformerRef->get_plug_by_URI(*(outputPlug->get_URI())) != NULL);
    assert(stagePerformerRef->get_plug_by_URI(*(inputPlug->get_URI())) != NULL);
	assert(stagePerformerRef->get_plug_by_URI(*(typedIntPlug->get_URI())) != NULL);

	//Check local client registered plugs correctly
	ZstPlug *localPlug = Showtime::get_performer_by_URI(outURI)->get_plug_by_URI(*outURI);
	assert(localPlug->get_URI() == outURI);

	//Check plug destruction
	Showtime::endpoint().destroy_plug(outputPlug);
	assert(stage->get_performer_ref_by_name(outURI_stack.performer_char())->get_plug_by_URI(outURI_stack) == NULL);
	
	Showtime::endpoint().destroy_plug(inputPlug);
	assert(stage->get_performer_ref_by_name(inURI_stack.performer_char())->get_plug_by_URI(inURI_stack) == NULL);

	Showtime::endpoint().destroy_plug(typedIntPlug);
	assert(stage->get_performer_ref_by_name(typedIntPlug_stack.performer_char())->get_plug_by_URI(typedIntPlug_stack) == NULL);

	TAKE_A_BREATH
	assert(Showtime::event_queue_size() > 0);
	Showtime::poll_once();
	assert(Showtime::event_queue_size() == 0);
	assert(stageEventHits == 3);
	stageEventHits = 0;
}

void test_memory_leaks() {
	ZstURI * outURI = ZstURI::create("test_performer_1", "mem_instrument", "mem_output_plug", ZstURI::Direction::OUT_JACK);
	ZstURI * inURI = ZstURI::create("test_performer_1", "mem_instrument", "mem_input_plug", ZstURI::Direction::IN_JACK);
	ZstIntPlug *output_int_plug = Showtime::create_int_plug(outURI);
	ZstIntPlug *input_int_plug = Showtime::create_int_plug(inURI);
	input_int_plug->attach_recv_callback(new TestIntValueCallback());
	Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI());

	int count = 99000;
	int current = 0;
	while (++current < count) {
		output_int_plug->fire(current);
		Showtime::poll_once();
	}

	TAKE_A_BREATH
	Showtime::destroy_plug(output_int_plug);
	Showtime::destroy_plug(input_int_plug);
}

void test_connect_plugs() {

	ZstURI * outURI = ZstURI::create("test_performer_1", "test_instrument", "test_output_plug", ZstURI::Direction::OUT_JACK);
	ZstURI * inURI = ZstURI::create("test_performer_1", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);
	ZstURI * badURI = ZstURI::create("fake_performer", "test_instrument", "test_input_plug", ZstURI::Direction::IN_JACK);

	//Test plugs connected between performers
	ZstIntPlug *output_int_plug = Showtime::create_int_plug(outURI);
	ZstIntPlug *input_int_plug = Showtime::create_int_plug(inURI);
    input_int_plug->attach_recv_callback(new TestIntValueCallback());
	Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI());

	TAKE_A_BREATH
	assert(Showtime::event_queue_size() > 0);
	Showtime::poll_once();
	assert(Showtime::event_queue_size() == 0);
	//assert(connectionCallbacks == 1);
	connectionCallbacks = 0;

	//Test connecting missing URI objects
	std::cout << " Testing bad cable connection request" << std::endl;
	assert(Showtime::connect_cable(output_int_plug->get_URI(), badURI) <= 0);
	delete badURI;
	std::cout << "Finished testing bad cable connection request" << std::endl;

	//Test plug value callbacks
	int num_fires = 5;
	for (int i = 0; i < num_fires; ++i) {
		output_int_plug->fire(i);
	}

	TAKE_A_BREATH
	assert(Showtime::event_queue_size() > 0);
	Showtime::poll_once();
	assert(Showtime::event_queue_size() == 0);
	assert(intPlugCallbacks == num_fires);
	intPlugCallbacks = 0;

	//Test manual event queue pop. This is thread safe so we can pop each event off at our leisure
	num_fires = 5;
	for (int i = 0; i < num_fires; ++i) {
		output_int_plug->fire(i);
	}

	TAKE_A_BREATH
	for (int i = num_fires; i > 0; --i) {
		assert(Showtime::event_queue_size() == i);
		ZstEvent e = Showtime::pop_event();
		assert(e.get_update_type() == ZstEvent::EventType::PLUG_HIT);
		assert(strcmp(e.get_first().to_char(), input_int_plug->get_URI()->to_char()) == 0);
		assert(Showtime::event_queue_size() == i-1);
	}
	assert(Showtime::event_queue_size() == 0);

	std::cout << "Queue test successful" << std::endl;

	//Testing connection disconnect and reconnect
	assert(Showtime::destroy_cable(output_int_plug->get_URI(), input_int_plug->get_URI()));
	TAKE_A_BREATH
	assert(Showtime::endpoint().get_cable_by_URI(*outURI, *inURI) == NULL);
	assert(Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI()));

	//Test plug destruction
	assert(Showtime::destroy_plug(output_int_plug));
	assert(Showtime::destroy_plug(input_int_plug));
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

	TAKE_A_BREATH
	test_URI();
	test_performer_init();
    test_stage_registration();
    test_create_plugs();
	test_connect_plugs();
	test_memory_leaks();
	test_cleanup();
	std::cout << "\nShowtime test successful" << std::endl;

#ifdef WIN32
	system("pause");
#endif
	return 0;
}
