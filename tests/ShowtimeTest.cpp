#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstPerformer.h"
#include "ZstStage.h"
#include "ZstEndpoint.h"
#include "ZstValue.h"

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 100);
#endif

#ifdef WIN32
#define WAIT_FOR_HEARTBEAT Sleep(HEARTBEAT_DURATION);
#else
#define WAIT_FOR_HEARTBEAT usleep(1000 * HEARTBEAT_DURATION);
#endif

inline void wait_for_poll() {
	while (Showtime::event_queue_size() < 1) {
		TAKE_A_BREATH 
	}
	Showtime::poll_once();
}

//Callback classes
//----------------
class TestPerformerArrivingEventCallback : public ZstPerformerEventCallback {
public:
	int performerArrivedHits = 0;
	std::string last_created_performer;
	void run(ZstURI performer) override {
		std::cout << "ZST_TEST CALLBACK - performer arriving " << performer.to_char() << std::endl;
		performerArrivedHits++;
		last_created_performer = std::string(performer.performer_char());
	}
	void reset() { performerArrivedHits = 0; }
};

class TestPerformerLeavingEventCallback : public ZstPerformerEventCallback {
public:
	int performerLeavingHits = 0;
	void run(ZstURI performer) override {
		std::cout << "ZST_TEST CALLBACK - performer leaving " << performer.to_char() << std::endl;
		performerLeavingHits++;
	}
	void reset() { performerLeavingHits = 0; }
};

// ----

class TestPlugArrivingEventCallback : public ZstPlugEventCallback {
public:
	int plugArrivedHits = 0;
	void run(ZstURI plug) override {
		std::cout << "ZST_TEST CALLBACK - plug arriving " << plug.to_char() << std::endl;
		plugArrivedHits++;
	}
	void reset() { plugArrivedHits = 0; }
};

class TestPlugLeavingEventCallback : public ZstPlugEventCallback {
public:
	int plugLeavingHits = 0;
	void run(ZstURI plug) override {
		std::cout << "ZST_TEST CALLBACK - plug leaving " << plug.to_char() << std::endl;
		plugLeavingHits++;
	}
	void reset() { plugLeavingHits = 0; }
};

// ----

class TestCableArrivingEventCallback : public ZstCableEventCallback {
public:
	int cableArrivedHits = 0;
	void run(ZstCable cable) override {
		std::cout << "ZST_TEST CALLBACK - cable arriving " << cable.get_output().to_char() << " to " << cable.get_input().to_char() << std::endl;
		cableArrivedHits++;
	}
	void reset() { cableArrivedHits = 0; }
};

class TestCableLeavingEventCallback : public ZstCableEventCallback {
public:
	int cableLeavingHits = 0;
	void run(ZstCable cable) override {
		std::cout << "ZST_TEST CALLBACK - cable leaving " << cable.get_output().to_char() << " to " << cable.get_input().to_char() << std::endl;
		cableLeavingHits++;
	}
	void reset() { cableLeavingHits = 0; }
};

// ----

class TestConnectionCallback : public ZstEventCallback {
public:
	int connectionCallbacks = 0;
	void run(ZstEvent e) override {
		connectionCallbacks++;
	};
};

class TestIntValueCallback : public ZstPlugDataEventCallback {
public:
	int compare_val;
	int intPlugCallbacks = 0;
	TestIntValueCallback(int cmpr) : compare_val(cmpr) {}
	void run(ZstInputPlug * plug) override {
		std::cout << "ZST_TEST CALLBACK - int: " << plug->value()->int_at(0) << std::endl;
		assert(plug->value()->int_at(0) == compare_val);
		intPlugCallbacks++;
	};
};

class TestFloatValueCallback : public ZstPlugDataEventCallback {
public:
	float compare_val;
	TestFloatValueCallback(float cmpr) : compare_val(cmpr) {}
	void run(ZstInputPlug * plug) override {
		std::cout << "ZST_TEST CALLBACK - float:" << plug->value()->float_at(0) << std::endl;
		assert(fabs(plug->value()->float_at(0) - compare_val) < FLT_EPSILON);
	};
};

class TestCharValueCallback : public ZstPlugDataEventCallback {
public:
	const char* compare_val;
	TestCharValueCallback(const char* cmpr) : compare_val(cmpr) {}
	void run(ZstInputPlug * plug) override {
		char * str_val = new char[plug->value()->size_at(0) + 1]();
		plug->value()->char_at(str_val, 0);
		std::cout << "ZST_TEST CALLBACK - char: " << str_val << std::endl;
		assert(strcmp(str_val, compare_val) == 0);
		delete[] str_val;
	};
};

class TestMultipleIntValueCallback : public ZstPlugDataEventCallback {
public:
	int compare_val;
	int _num_values;
	TestMultipleIntValueCallback(int cmpr, int num_values) : compare_val(cmpr), _num_values(num_values) {}
	void run(ZstInputPlug * plug) override {
		std::cout << "ZST_TEST CALLBACK - multple ints" << std::endl;
		assert(plug->value()->size() == _num_values);
		for (int i = 0; i < plug->value()->size(); ++i) {
			assert(plug->value()->int_at(i));
		}
	};
};



//Global test variables
ZstStage *stage;
ZstPerformer *performer_a;

void test_standard_layout() {
	//Verify standard layout
	assert(std::is_standard_layout<ZstURI>());
	assert(std::is_standard_layout<ZstCable>());
	assert(std::is_standard_layout<ZstEvent>());
}

void test_URI() {
	std::cout << "Starting URI test" << std::endl;

	ZstURI uri_empty = ZstURI();
	ZstURI uri_equal1 = ZstURI("perf", "ins", "someplug");
	ZstURI uri_notequal = ZstURI("perf", "anotherins", "someplug");

	assert(uri_empty.is_empty());
	assert(uri_equal1 == uri_equal1);
	assert(uri_equal1 != uri_notequal);
    assert(strcmp(uri_equal1.to_char() , "perf/ins/someplug") == 0);

	//Test ZstCable equality against ZstURI instances
	ZstCable test_cable_eq1 = ZstCable(uri_equal1, uri_empty);
	ZstCable test_cable_neq1 = ZstCable(uri_equal1, uri_notequal);

	assert(test_cable_eq1 == test_cable_eq1);
	assert(test_cable_eq1 != test_cable_neq1);
	assert(test_cable_eq1.is_attached(uri_equal1));
	assert(test_cable_eq1.is_attached(uri_empty));
	assert(!(test_cable_neq1.is_attached(uri_empty)));

	std::cout << "Finished URI test\n" << std::endl;
}

void test_performer_init() {
    //Test single performer init
	std::cout << "Starting performer init test" << std::endl;
	Showtime::endpoint().self_test();

	TestPerformerArrivingEventCallback * perfArriveCallback = new TestPerformerArrivingEventCallback();
	Showtime::attach_performer_arriving_callback(perfArriveCallback);

	performer_a = Showtime::create_performer("test_performer_1");
	assert(performer_a);
    wait_for_poll();
	assert(perfArriveCallback->performerArrivedHits == 1);
	assert(perfArriveCallback->last_created_performer == performer_a->get_name());
	perfArriveCallback->reset();

	Showtime::remove_performer_arriving_callback(perfArriveCallback);
	delete perfArriveCallback;

	std::cout << "Finished performer init test\n" << std::endl;
}

//Test stage creation and performer
void test_stage_registration(){
	std::cout << "Starting stage registration test" << std::endl;

    //Test stage connection
    assert(Showtime::endpoint().ping_stage() >= 0);
    assert(stage->get_performer_ref_by_name("test_performer_1") != NULL);
    assert(stage->get_performer_ref_by_name("non_existing_performer") == NULL);

	std::cout << "Finished stage registration test\n" << std::endl;
}

void test_create_plugs(){
	std::cout << "Starting create plugs test" << std::endl;

	TestPlugArrivingEventCallback * plugArrivalCallback = new TestPlugArrivingEventCallback();
	Showtime::attach_plug_arriving_callback(plugArrivalCallback);

    //Create new plugs
	ZstURI outURI = ZstURI("test_performer_1", "test_instrument", "test_output_plug");
	ZstURI inURI = ZstURI("test_performer_1", "test_instrument", "test_input_plug");

	//URI ownership taken over by plug
	ZstOutputPlug *outputPlug = Showtime::create_output_plug(outURI, ZstValueType::ZST_INT);
	ZstInputPlug *inputPlug = Showtime::create_input_plug(inURI, ZstValueType::ZST_INT);
    
    //Check address equality
    assert(outputPlug->get_URI() == outURI);
    assert(!(outputPlug->get_URI() == inURI));

	//Test creating plug with type functions (blame swig python!)
	ZstInputPlug *typedInputPlug = Showtime::create_input_plug(ZstURI("test_performer_1", "test_instrument", "test_int_plug"), ZstValueType::ZST_INT);
	ZstURI typedIntPlug_stack = ZstURI(typedInputPlug->get_URI());

	//Check that our plug creation callbacks fired successfully
	wait_for_poll();
	assert(plugArrivalCallback->plugArrivedHits == 3);
	plugArrivalCallback->reset();
	Showtime::remove_plug_arriving_callback(plugArrivalCallback);
	delete plugArrivalCallback;

    //Check stage registered plugs successfully
    ZstPerformerRef *stagePerformerRef = stage->get_performer_ref_by_name("test_performer_1");
    assert(stagePerformerRef->get_plug_by_URI(outputPlug->get_URI()) != NULL);
    assert(stagePerformerRef->get_plug_by_URI(inputPlug->get_URI()) != NULL);
	assert(stagePerformerRef->get_plug_by_URI(typedInputPlug->get_URI()) != NULL);

	//Check local client registered plugs correctly
	ZstPlug *localPlug = Showtime::get_performer_by_URI(outURI)->get_plug_by_URI(outURI);
	assert(localPlug->get_URI() == outURI);

	//Check plug destruction
	TestPlugLeavingEventCallback * plugLeavingCallback = new TestPlugLeavingEventCallback();
	Showtime::attach_plug_leaving_callback(plugLeavingCallback);

	Showtime::endpoint().destroy_plug(outputPlug);
	assert(stage->get_performer_ref_by_name(outURI.performer_char())->get_plug_by_URI(outURI) == NULL);
	
	Showtime::endpoint().destroy_plug(inputPlug);
	assert(stage->get_performer_ref_by_name(inURI.performer_char())->get_plug_by_URI(inURI) == NULL);

	Showtime::endpoint().destroy_plug(typedInputPlug);
	assert(stage->get_performer_ref_by_name(typedIntPlug_stack.performer_char())->get_plug_by_URI(typedIntPlug_stack) == NULL);

	wait_for_poll();
	assert(plugLeavingCallback->plugLeavingHits == 3);
	plugLeavingCallback->reset();
	Showtime::remove_plug_leaving_callback(plugLeavingCallback);

	std::cout << "Finished create plugs test\n" << std::endl;
}


void test_connect_plugs() {
	std::cout << "Starting connect plugs test" << std::endl;

	//TestConnectionCallback * cableCallback = new TestConnectionCallback();
	//Showtime::attach_cable_callback(cableCallback);
	ZstURI outURI = ZstURI("test_performer_1", "test_instrument", "test_output_plug");
	ZstURI inURI = ZstURI("test_performer_1", "test_instrument", "test_input_plug");
	ZstURI badURI = ZstURI("fake_performer", "test_instrument", "test_input_plug");

	int int_fire_val = 27;

	//Test plugs connected between performers
	ZstOutputPlug *output_int_plug = Showtime::create_output_plug(outURI, ZstValueType::ZST_INT);
	ZstInputPlug *input_int_plug = Showtime::create_input_plug(inURI, ZstValueType::ZST_INT);
	input_int_plug->input_events()->attach_event_callback(new TestIntValueCallback(int_fire_val));
	
	//Test cable callbacks
	TestCableArrivingEventCallback * cableArriveCallback = new TestCableArrivingEventCallback();
	TestCableLeavingEventCallback * cableLeaveCallback = new TestCableLeavingEventCallback();

	Showtime::attach_cable_arriving_callback(cableArriveCallback);
	Showtime::attach_cable_leaving_callback(cableLeaveCallback);
	Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI());
	wait_for_poll();
	assert(cableArriveCallback->cableArrivedHits == 1);
	cableArriveCallback->reset();
	assert(Showtime::endpoint().get_cable_by_URI(outURI, inURI) != NULL);

	//Test connecting missing URI objects
	std::cout << " Testing bad cable connection request" << std::endl;
	assert(Showtime::connect_cable(output_int_plug->get_URI(), badURI) <= 0);
	std::cout << "Finished testing bad cable connection request" << std::endl;

	//Testing connection disconnect and reconnect
	assert(Showtime::destroy_cable(output_int_plug->get_URI(), input_int_plug->get_URI()));
	wait_for_poll();
	assert(cableLeaveCallback->cableLeavingHits == 1);
	cableLeaveCallback->reset();
	assert(Showtime::endpoint().get_cable_by_URI(outURI, inURI) == NULL);
	assert(Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI()));

	//Test plug and cable destruction
	assert(Showtime::destroy_plug(output_int_plug));
	wait_for_poll();
	assert(cableLeaveCallback->cableLeavingHits == 1);
	cableLeaveCallback->reset();
	assert(Showtime::destroy_plug(input_int_plug));

	//Test removing callbacks
	Showtime::remove_cable_arriving_callback(cableArriveCallback);
	Showtime::remove_cable_leaving_callback(cableLeaveCallback);
	delete cableArriveCallback;
	delete cableLeaveCallback;

	std::cout << "Finished connect plugs test\n" << std::endl;
}



void test_check_plug_values() {
	std::cout << "Starting check plug values test" << std::endl;

	ZstURI outURI = ZstURI("test_performer_1", "mem_instrument", "mem_output_plug");
	ZstURI inURI = ZstURI("test_performer_1", "mem_instrument", "mem_input_plug");
	ZstOutputPlug *output_plug = Showtime::create_output_plug(outURI, ZstValueType::ZST_INT);
	ZstInputPlug *input_plug = Showtime::create_input_plug(inURI, ZstValueType::ZST_INT);
	Showtime::connect_cable(output_plug->get_URI(), input_plug->get_URI());

	TAKE_A_BREATH
	wait_for_poll();

	//Test manual event queue pop. This is thread safe so we can pop each event off at our leisure
	std::cout << "Starting Queue test" << std::endl;
	assert(Showtime::event_queue_size() == 0);
	int num_fires = 5;
	for (int i = 0; i < num_fires; ++i) {
		output_plug->value()->append_int(1);
		output_plug->fire();
		output_plug->value()->clear();
	}
	TAKE_A_BREATH

	int queue_size = Showtime::event_queue_size();

	for (int i = num_fires; i > 0; --i) {
		assert(Showtime::event_queue_size() == i);
		ZstEvent e = Showtime::pop_event();
		assert(e.get_update_type() == ZstEvent::EventType::PLUG_HIT);
		assert(strcmp(e.get_first().to_char(), input_plug->get_URI().to_char()) == 0);
		assert(Showtime::event_queue_size() == i - 1);
	}
	assert(Showtime::event_queue_size() == 0);
	std::cout << "Finished Queue test\n" << std::endl;


	//Test int value conversion
	std::cout << "Testing int ZstValues via callback" << std::endl;
	int int_cmpr_val = 27;
	TestIntValueCallback * int_callback = new TestIntValueCallback(int_cmpr_val);
	input_plug->input_events()->attach_event_callback(int_callback);
	output_plug->value()->append_int(int_cmpr_val);
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	output_plug->value()->append_float(float(int_cmpr_val));
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	output_plug->value()->append_char(std::to_string(int_cmpr_val).c_str());
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	int_callback->compare_val = 0;
	output_plug->value()->append_char("not a number");
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	input_plug->input_events()->remove_event_callback(int_callback);


	//Test float value conversion
	std::cout << "Testing float ZstValues via callback" << std::endl;
	float float_cmpr_val = 28.5f;
	TestFloatValueCallback * float_callback = new TestFloatValueCallback(float_cmpr_val);
	input_plug->input_events()->attach_event_callback(float_callback);
	output_plug->value()->append_float(float_cmpr_val);
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();
	
	output_plug->value()->append_char(std::to_string(float_cmpr_val).c_str());
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	float_callback->compare_val = 0.0f;
	output_plug->value()->append_char("not a number");
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	float_callback->compare_val = int(float_cmpr_val + 0.5f);
	output_plug->value()->append_int(int(float_cmpr_val + 0.5f)); //Rounded int to match the interal value conversion
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	input_plug->input_events()->remove_event_callback(float_callback);


	//Test char* value conversion
	std::string char_cmp_val = "hello world";
	std::cout << "Testing char ZstValues via callback" << std::endl;
	TestCharValueCallback * char_callback = new TestCharValueCallback(char_cmp_val.c_str());
	input_plug->input_events()->attach_event_callback(char_callback);
	output_plug->value()->append_char(char_cmp_val.c_str());
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	//delete[] char_callback->compare_val;
	char_callback->compare_val = "29";
	output_plug->value()->append_int(29);
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	//delete[] char_callback->compare_val;
	char_callback->compare_val = "29.50000";
	output_plug->value()->append_float(29.5f);
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();

	input_plug->input_events()->remove_event_callback(char_callback);


	//Test multiple values
	int_cmpr_val = 30;
	int int_num_value = 10;
	TestMultipleIntValueCallback * int_mult_callback = new TestMultipleIntValueCallback(int_cmpr_val, int_num_value);
	input_plug->input_events()->attach_event_callback(int_mult_callback);

	for (int i = 0; i < int_num_value; ++i) {
		output_plug->value()->append_int(int_cmpr_val);
	}
	output_plug->fire();
	output_plug->value()->clear();
	wait_for_poll();
	input_plug->input_events()->remove_event_callback(int_mult_callback);

	Showtime::destroy_plug(output_plug);
	Showtime::destroy_plug(input_plug);

	std::cout << "Finished check plug values test\n" << std::endl;
}


void test_memory_leaks() {
	std::cout << "Starting plug fire memory leak test" << std::endl;

	ZstURI outURI = ZstURI("test_performer_1", "mem_instrument", "mem_output_plug");
	ZstURI inURI = ZstURI("test_performer_1", "mem_instrument", "mem_input_plug");
	ZstOutputPlug *output_int_plug = Showtime::create_output_plug(outURI, ZstValueType::ZST_INT);
	ZstInputPlug *input_int_plug = Showtime::create_input_plug(inURI, ZstValueType::ZST_INT);

	int int_cmpr_val = 10;
	TestIntValueCallback * int_callback = new TestIntValueCallback(int_cmpr_val);
	input_int_plug->input_events()->attach_event_callback(int_callback);
	Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI());

	int count = 9999;
	int current = 0;
	while (++current < count) {
		output_int_plug->value()->append_int(int_cmpr_val);
		output_int_plug->fire();
		output_int_plug->value()->clear();
	}
	wait_for_poll();

	Showtime::destroy_plug(output_int_plug);
	Showtime::destroy_plug(input_int_plug);

	std::cout << "Starting check plug values test" << std::endl;

}

void test_leaving(){
    Showtime::leave();
    TAKE_A_BREATH
    assert(stage->get_performer_ref_by_name("test_performer_1") == NULL);
    TAKE_A_BREATH
}


void test_cleanup() {
	delete stage;
    stage = NULL;
	Showtime::destroy();
}

int main(int argc,char **argv){
    stage = ZstStage::create_stage();
	std::cout << "Stage created" << std::endl;

	Showtime::init();
    Showtime::join("127.0.0.1");

	TAKE_A_BREATH
	test_standard_layout();
	test_URI();
	test_performer_init();
    test_stage_registration();
    test_create_plugs();
	test_connect_plugs();
	test_check_plug_values();
	test_memory_leaks();
    test_leaving();
	test_cleanup();
	std::cout << "\nShowtime test successful" << std::endl;

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
