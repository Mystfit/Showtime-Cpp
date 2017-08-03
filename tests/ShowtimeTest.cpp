#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstStage.h"
#include "ZstEndpoint.h"
#include "ZstValue.h"
#include "entities/AddFilter.h"
#include "entities/ZstFilter.h"
#include "entities/ZstPatch.h"

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 200);
#endif

#ifdef WIN32
#define WAIT_FOR_HEARTBEAT Sleep(HEARTBEAT_DURATION);
#else
#define WAIT_FOR_HEARTBEAT usleep(1000 * HEARTBEAT_DURATION);
#endif

inline void wait_for_callbacks(int expected_messages) {
	while (Showtime::event_queue_size() < expected_messages) {
		TAKE_A_BREATH 
	}
	Showtime::poll_once();
}

inline void clear_callback_queue() {
	Showtime::poll_once();
}

//Callback classes
//----------------
class TestEntityArrivingEventCallback : public ZstEntityEventCallback {
public:
	int entityArrivedHits = 0;
	std::string last_created_entity;
	void run(ZstURI entity) override {
		std::cout << "ZST_TEST CALLBACK - entity arriving " << entity.path() << std::endl;
		entityArrivedHits++;
		last_created_entity = std::string(entity.path());
	}
	void reset() { entityArrivedHits = 0; }
};

class TestEntityLeavingEventCallback : public ZstEntityEventCallback {
public:
	int entityLeavingHits = 0;
	void run(ZstURI entity) override {
		std::cout << "ZST_TEST CALLBACK - entity leaving " << entity.path() << std::endl;
		entityLeavingHits++;
	}
	void reset() { entityLeavingHits = 0; }
};

// ----

class TestPlugArrivingEventCallback : public ZstPlugEventCallback {
public:
	int plugArrivedHits = 0;
	void run(ZstURI plug) override {
		std::cout << "ZST_TEST CALLBACK - plug arriving " << plug.path() << std::endl;
		plugArrivedHits++;
	}
	void reset() { plugArrivedHits = 0; }
};

class TestPlugLeavingEventCallback : public ZstPlugEventCallback {
public:
	int plugLeavingHits = 0;
	void run(ZstURI plug) override {
		std::cout << "ZST_TEST CALLBACK - plug leaving " << plug.path() << std::endl;
		plugLeavingHits++;
	}
	void reset() { plugLeavingHits = 0; }
};

// ----

class TestCableArrivingEventCallback : public ZstCableEventCallback {
public:
	int cableArrivedHits = 0;
	void run(ZstCable cable) override {
		std::cout << "ZST_TEST CALLBACK - cable arriving " << cable.get_output().path() << " to " << cable.get_input().path() << std::endl;
		cableArrivedHits++;
	}
	void reset() { cableArrivedHits = 0; }
};

class TestCableLeavingEventCallback : public ZstCableEventCallback {
public:
	int cableLeavingHits = 0;
	void run(ZstCable cable) override {
		std::cout << "ZST_TEST CALLBACK - cable leaving " << cable.get_output().path() << " to " << cable.get_input().path() << std::endl;
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
		//std::cout << "ZST_TEST CALLBACK - int: " << plug->value().int_at(0) << std::endl;
		assert(plug->value().int_at(0) == compare_val);
		intPlugCallbacks++;
	};
};

class TestFloatValueCallback : public ZstPlugDataEventCallback {
public:
	float compare_val;
	TestFloatValueCallback(float cmpr) : compare_val(cmpr) {}
	void run(ZstInputPlug * plug) override {
		//std::cout << "ZST_TEST CALLBACK - float:" << plug->value().float_at(0) << std::endl;
		assert(fabs(plug->value().float_at(0) - compare_val) < FLT_EPSILON);
	};
};

class TestCharValueCallback : public ZstPlugDataEventCallback {
public:
	const char* compare_val;
	TestCharValueCallback(const char* cmpr) : compare_val(cmpr) {}
	void run(ZstInputPlug * plug) override {
		char * str_val = new char[plug->value().size_at(0) + 1]();
		plug->value().char_at(str_val, 0);
		//std::cout << "ZST_TEST CALLBACK - char: " << str_val << std::endl;
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
		//std::cout << "ZST_TEST CALLBACK - multple ints" << std::endl;
		assert(plug->value().size() == _num_values);
		for (int i = 0; i < plug->value().size(); ++i) {
			assert(plug->value().int_at(i));
		}
	};
};

class AddFilterSumCallback : public ZstPlugDataEventCallback {
public:
	int expected_sum;
	AddFilterSumCallback(int sum) : expected_sum(sum) {}

	void run(ZstInputPlug * plug) override {
		std::cout << "ZST_TEST CALLBACK - addition filter" << std::endl;
		std::cout << "ZST_TEST CALLBACK - expected " << expected_sum << " - received " << plug->value().int_at(0) << std::endl;
		assert(plug->value().int_at(0) == expected_sum);
	}
};



//Global test variables
ZstStage *stage;
ZstPatch * root_entity;

void test_standard_layout() {
	//Verify standard layout
	assert(std::is_standard_layout<ZstURI>());
	assert(std::is_standard_layout<ZstCable>());
	assert(std::is_standard_layout<ZstEvent>());
}

void test_URI() {
	std::cout << "Starting URI test" << std::endl;

	ZstURI uri_empty = ZstURI();
	ZstURI uri_equal1 = ZstURI("ins/someplug");
	ZstURI uri_notequal = ZstURI("anotherins/someplug");

	assert(uri_empty.is_empty());
	assert(uri_equal1 == uri_equal1);
	assert(uri_equal1 != uri_notequal);
    assert(strcmp(uri_equal1.path(), "ins/someplug") == 0);

	assert(ZstURI("root_entity/filter") == ZstURI("root_entity/filter"));
	assert(!(ZstURI("root_entity") == ZstURI("root_entity/filter")));
	assert(ZstURI("root_entity") != ZstURI("root_entity/filter"));
	assert(ZstURI("b") < ZstURI("c"));
	assert(ZstURI("a") < ZstURI("b"));
	assert(ZstURI("a") < ZstURI("c"));
	assert(uri_equal1.size() == 2);

	//Test URI going out of scope
	{
		ZstURI stack_uri("some_entity/some_name");
	}

	assert(strcmp(uri_equal1[0], "ins") == 0);
	assert(strcmp(uri_equal1[1], "someplug") == 0);
	bool thrown_range_error = false;
	try {
		uri_equal1[2];
	}
	catch (std::range_error){
		thrown_range_error = true;
	}
	assert(thrown_range_error);

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

void test_root_entity() {
    //Test single entity init
	std::cout << "Starting entity init test" << std::endl;
	Showtime::endpoint().self_test();

	TestEntityArrivingEventCallback * entityArriveCallback = new TestEntityArrivingEventCallback();
	Showtime::attach_entity_arriving_callback(entityArriveCallback);

	root_entity = new ZstPatch("root_entity");
	assert(root_entity);
    wait_for_callbacks(1);
	assert(entityArriveCallback->entityArrivedHits == 1);
	assert(entityArriveCallback->last_created_entity == std::string(root_entity->URI().path()));
	entityArriveCallback->reset();

	Showtime::remove_performer_arriving_callback(entityArriveCallback);
	delete entityArriveCallback;
	clear_callback_queue();

	std::cout << "Finished entity init test\n" << std::endl;
}

//Test stage creation and entity
void test_stage_registration(){
	std::cout << "Starting stage registration test" << std::endl;

    //Test stage connection
    assert(Showtime::endpoint().ping_stage() >= 0);
	assert(stage->get_endpoint_ref_by_UUID(Showtime::endpoint().get_endpoint_UUID()) != NULL);

	std::cout << "Finished stage registration test\n" << std::endl;
}

void test_create_plugs(){
	std::cout << "Starting create plugs test" << std::endl;

	//Attach stage level callback to watch for arriving plugs
	TestPlugArrivingEventCallback * plugArrivalCallback = new TestPlugArrivingEventCallback();
	Showtime::attach_plug_arriving_callback(plugArrivalCallback);

	//Create a filter to hold out test plugs
	std::string filter_name = "test_filter";
	ZstFilter * test_filter = new ZstFilter(filter_name.c_str(), root_entity);
	wait_for_callbacks(1);

	//URI ownership taken over by plug
	ZstOutputPlug *outputPlug = test_filter->create_output_plug("test_out", ZstValueType::ZST_INT);
	ZstInputPlug *inputPlug = test_filter->create_input_plug("test_in", ZstValueType::ZST_INT);
    
    //Check address equality
	assert(outputPlug->get_URI() == ZstURI("root_entity/test_filter/test_out"));
    assert(!(outputPlug->get_URI() == ZstURI("root_entity/test_filter/test_in")));

	//Test creating plug with type functions (blame swig python!)
	ZstInputPlug *typedInputPlug = test_filter->create_input_plug("test_int_plug", ZstValueType::ZST_INT);

	//Check that our plug creation callbacks fired successfully
	wait_for_callbacks(3);
	assert(plugArrivalCallback->plugArrivedHits == 3);
	plugArrivalCallback->reset();
	Showtime::remove_plug_arriving_callback(plugArrivalCallback);
	delete plugArrivalCallback;

    //Check stage registered plugs successfully
    assert(stage->get_plug_by_URI(outputPlug->get_URI()) != NULL);
    assert(stage->get_plug_by_URI(inputPlug->get_URI()) != NULL);
	assert(stage->get_plug_by_URI(typedInputPlug->get_URI()) != NULL);

	//Check local client registered plugs correctly
	ZstPlug *localPlug = test_filter->get_plug_by_URI(outputPlug->get_URI());
	assert(localPlug->get_URI() == outputPlug->get_URI());

	//Check plug destruction
	TestPlugLeavingEventCallback * plugLeavingCallback = new TestPlugLeavingEventCallback();
	Showtime::attach_plug_leaving_callback(plugLeavingCallback);

	//Test single plug destruction
	ZstURI outURI = outputPlug->get_URI();
	test_filter->remove_plug(outputPlug);
	wait_for_callbacks(1);
	assert(stage->get_plug_by_URI(outURI) == NULL);

	//Test plug destruction when destroying entity
	delete test_filter;
	wait_for_callbacks(3);

	ZstURI inURI = inputPlug->get_URI();
	assert(stage->get_plug_by_URI(inURI) == NULL);

	ZstURI typedURI = typedInputPlug->get_URI();
	assert(stage->get_plug_by_URI(typedURI) == NULL);

	//Make sure that the plug leaving callback was hit the correct number of times
	assert(plugLeavingCallback->plugLeavingHits == 3);
	plugLeavingCallback->reset();
	Showtime::remove_plug_leaving_callback(plugLeavingCallback);
	clear_callback_queue();

	std::cout << "Finished create plugs test\n" << std::endl;
}


void test_connect_plugs() {
	std::cout << "Starting connect plugs test" << std::endl;

	int int_fire_val = 27;

	std::string filter_name = "test_filter";
	ZstFilter * test_filter = new ZstFilter(filter_name.c_str(), root_entity);

	//Test plugs connected between performers
	ZstOutputPlug *output_int_plug = test_filter->create_output_plug("test_output_plug", ZstValueType::ZST_INT);
	ZstInputPlug *input_int_plug = test_filter->create_input_plug("test_input_plug", ZstValueType::ZST_INT);
	input_int_plug->attach_receive_callback(new TestIntValueCallback(int_fire_val));
	
	//Test cable callbacks
	TestCableArrivingEventCallback * cableArriveCallback = new TestCableArrivingEventCallback();
	TestCableLeavingEventCallback * cableLeaveCallback = new TestCableLeavingEventCallback();

	Showtime::attach_cable_arriving_callback(cableArriveCallback);
	Showtime::attach_cable_leaving_callback(cableLeaveCallback);
	Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI());
	wait_for_callbacks(4);
	assert(cableArriveCallback->cableArrivedHits == 1);
	cableArriveCallback->reset();
	assert(Showtime::endpoint().get_cable_by_URI(output_int_plug->get_URI(), input_int_plug->get_URI()) != NULL);

	//Test connecting missing URI objects
	ZstURI badURI = ZstURI("test_instrument/bad_plug");
	std::cout << " Testing bad cable connection request" << std::endl;
	assert(Showtime::connect_cable(output_int_plug->get_URI(), badURI) <= 0);
	std::cout << "Finished testing bad cable connection request" << std::endl;

	//Testing connection disconnect and reconnect
	assert(Showtime::destroy_cable(output_int_plug->get_URI(), input_int_plug->get_URI()));
	wait_for_callbacks(1);
	assert(cableLeaveCallback->cableLeavingHits == 1);
	cableLeaveCallback->reset();
	assert(Showtime::endpoint().get_cable_by_URI(output_int_plug->get_URI(), input_int_plug->get_URI()) == NULL);
	assert(Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI()));

	//Test plug destruction causes cable destruction
	test_filter->remove_plug(output_int_plug);
	output_int_plug = 0;
	wait_for_callbacks(2);
	assert(cableLeaveCallback->cableLeavingHits == 1);
	cableLeaveCallback->reset();
	test_filter->remove_plug(input_int_plug);
	input_int_plug = 0;

	//Test removing callbacks
	Showtime::remove_cable_arriving_callback(cableArriveCallback);
	Showtime::remove_cable_leaving_callback(cableLeaveCallback);
	delete cableArriveCallback;
	delete cableLeaveCallback;
	delete test_filter;
	clear_callback_queue();

	std::cout << "Finished connect plugs test\n" << std::endl;
}


void test_add_filter() {
	std::cout << "Starting addition filter test" << std::endl;
	
	//Create a test filter to hold out in/out plugs
	std::string filter_name = "test_filter";
	ZstFilter * filter = new ZstFilter(filter_name.c_str(), root_entity);
	wait_for_callbacks(1);

	//Create plugs
	ZstOutputPlug *augend_plug = filter->create_output_plug("add_test_out_A", ZstValueType::ZST_INT);
	ZstOutputPlug *addend_plug = filter->create_output_plug("add_test_out_B", ZstValueType::ZST_INT);
	ZstInputPlug *sum_plug = filter->create_input_plug("add_test_in_A", ZstValueType::ZST_INT);
	wait_for_callbacks(3);

	//Create a callback to listen for the sum result
	AddFilterSumCallback * sum_callback = new AddFilterSumCallback(4);
	sum_plug->attach_receive_callback(sum_callback);

	//Create the addition filter
	AddFilter * add_filter = new AddFilter(root_entity);
	wait_for_callbacks(4);

	//Connect cables to filter in/out
	Showtime::connect_cable(augend_plug->get_URI(), add_filter->augend()->get_URI());
	Showtime::connect_cable(addend_plug->get_URI(), add_filter->addend()->get_URI());
	Showtime::connect_cable(sum_plug->get_URI(), add_filter->sum()->get_URI());
	wait_for_callbacks(3);

	//Send values
	augend_plug->value().append_int(2);
	augend_plug->fire();
	addend_plug->value().append_int(2);
	addend_plug->fire();

	//Wait for the first two input callbacks to clear before we check for the sum
	wait_for_callbacks(2);
	TAKE_A_BREATH
	wait_for_callbacks(1);

	//Send more values
	sum_callback->expected_sum = 30;
	augend_plug->value().append_int(10);
	augend_plug->fire();
	addend_plug->value().append_int(20);
	addend_plug->fire();
	wait_for_callbacks(2);
	TAKE_A_BREATH
	wait_for_callbacks(1);

	//Cleanup
	sum_plug->remove_receive_callback(sum_callback);
	delete sum_callback;

	delete filter;
	augend_plug = 0;
	addend_plug = 0;
	sum_plug = 0;
	delete add_filter;
	clear_callback_queue();

	std::cout << "Finished addition filter test" << std::endl;
}


void test_check_plug_values() {
	std::cout << "Starting check plug values test" << std::endl;

	std::string filter_name = "test_filter";
	ZstFilter * test_filter = new ZstFilter(filter_name.c_str(), root_entity);

	ZstOutputPlug *output_plug = test_filter->create_output_plug("mem_test_out", ZstValueType::ZST_INT);
	ZstInputPlug *input_plug = test_filter->create_input_plug("mem_test_in", ZstValueType::ZST_INT);
	Showtime::connect_cable(output_plug->get_URI(), input_plug->get_URI());
	wait_for_callbacks(4);

	//Test manual event queue pop. This is thread safe so we can pop each event off at our leisure
	std::cout << "Starting Queue test" << std::endl;
	assert(Showtime::event_queue_size() == 0);
	int num_fires = 5;
	for (int i = 0; i < num_fires; ++i) {
		output_plug->value().append_int(1);
		output_plug->fire();
	}
	//Since we're testing the manual event queue system, we need a small delay so messages propogate back
	while (Showtime::event_queue_size() < num_fires) {}

	//Test manual queue event pop
	for (int i = num_fires; i > 0; --i) {
		assert(Showtime::event_queue_size() == i);
		ZstEvent e = Showtime::pop_event();
		assert(e.get_update_type() == ZstEvent::EventType::PLUG_HIT);
		assert(e.get_first() == input_plug->get_URI());
		assert(Showtime::event_queue_size() == i - 1);
	}
	assert(Showtime::event_queue_size() == 0);
	std::cout << "Processed " << num_fires << " fires" << std::endl;
	std::cout << "Finished Queue test\n" << std::endl;

	//Test int value conversion
	std::cout << "Testing int ZstValues via callback" << std::endl;
	int int_cmpr_val = 27;
	TestIntValueCallback * int_callback = new TestIntValueCallback(int_cmpr_val);
	input_plug->attach_receive_callback(int_callback);
	output_plug->value().append_int(int_cmpr_val);
	output_plug->fire();
	wait_for_callbacks(1);

	output_plug->value().append_float(float(int_cmpr_val));
	output_plug->fire();
	wait_for_callbacks(1);

	output_plug->value().append_char(std::to_string(int_cmpr_val).c_str());
	output_plug->fire();
	wait_for_callbacks(1);

	int_callback->compare_val = 0;
	output_plug->value().append_char("not a number");
	output_plug->fire();
	wait_for_callbacks(1);

	input_plug->remove_receive_callback(int_callback);


	//Test float value conversion
	std::cout << "Testing float ZstValues via callback" << std::endl;
	float float_cmpr_val = 28.5f;
	TestFloatValueCallback * float_callback = new TestFloatValueCallback(float_cmpr_val);
	input_plug->attach_receive_callback(float_callback);
	output_plug->value().append_float(float_cmpr_val);
	output_plug->fire();
	wait_for_callbacks(1);
	
	output_plug->value().append_char(std::to_string(float_cmpr_val).c_str());
	output_plug->fire();
	wait_for_callbacks(1);

	float_callback->compare_val = 0.0f;
	output_plug->value().append_char("not a number");
	output_plug->fire();
	wait_for_callbacks(1);

	float_callback->compare_val = int(float_cmpr_val + 0.5f);
	output_plug->value().append_int(int(float_cmpr_val + 0.5f)); //Rounded int to match the interal value conversion
	output_plug->fire();
	wait_for_callbacks(1);

	input_plug->remove_receive_callback(float_callback);


	//Test char* value conversion
	std::string char_cmp_val = "hello world";
	std::cout << "Testing char ZstValues via callback" << std::endl;
	TestCharValueCallback * char_callback = new TestCharValueCallback(char_cmp_val.c_str());
	input_plug->attach_receive_callback(char_callback);
	output_plug->value().append_char(char_cmp_val.c_str());
	output_plug->fire();
	wait_for_callbacks(1);

	//delete[] char_callback->compare_val;
	char_callback->compare_val = "29";
	output_plug->value().append_int(29);
	output_plug->fire();
	wait_for_callbacks(1);

	//delete[] char_callback->compare_val;
	char_callback->compare_val = "29.50000";
	output_plug->value().append_float(29.5f);
	output_plug->fire();
	wait_for_callbacks(1);

	input_plug->remove_receive_callback(char_callback);


	//Test multiple values
	int_cmpr_val = 30;
	int int_num_value = 10;
	TestMultipleIntValueCallback * int_mult_callback = new TestMultipleIntValueCallback(int_cmpr_val, int_num_value);
	input_plug->attach_receive_callback(int_mult_callback);

	for (int i = 0; i < int_num_value; ++i) {
		output_plug->value().append_int(int_cmpr_val);
	}
	output_plug->fire();
	wait_for_callbacks(1);

	//Cleanup
	input_plug->attach_receive_callback(int_mult_callback);
	delete test_filter;
	clear_callback_queue();

	std::cout << "Finished check plug values test\n" << std::endl;
}


void test_memory_leaks() {
	std::cout << "Starting plug fire memory leak test" << std::endl;

	std::string filter_name = "test_filter";
	ZstFilter * test_filter = new ZstFilter(filter_name.c_str(), root_entity);

	ZstOutputPlug *output_int_plug = test_filter->create_output_plug("mem_test_out", ZstValueType::ZST_INT);
	ZstInputPlug *input_int_plug = test_filter->create_input_plug("mem_test_in", ZstValueType::ZST_INT);

	int int_cmpr_val = 10;
	TestIntValueCallback * int_callback = new TestIntValueCallback(int_cmpr_val);
	input_int_plug->attach_receive_callback(int_callback);
	Showtime::connect_cable(output_int_plug->get_URI(), input_int_plug->get_URI());
	wait_for_callbacks(4);

	//Test sending large numbers of messages
	int count = 20000;
	for (int i = 0; i < count; ++i) {
		output_int_plug->value().append_int(int_cmpr_val);
		output_int_plug->fire();
		Showtime::poll_once();
	}

	delete test_filter;
	clear_callback_queue();

	std::cout << "Starting check plug values test" << std::endl;

}

void test_leaving(){
    Showtime::leave();
    TAKE_A_BREATH
    assert(stage->get_endpoint_ref_by_UUID(Showtime::endpoint().get_endpoint_UUID()) == NULL);
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
	test_root_entity();
    test_stage_registration();
    test_create_plugs();
	test_connect_plugs();
	test_add_filter();
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
