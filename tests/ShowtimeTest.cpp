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



class OutputComponent : public ZstComponent {
private:
    ZstOutputPlug * m_output;

public:
	OutputComponent(const char * name, ZstEntityBase * parent) : ZstComponent("TESTER", name, parent) {
		init();
	}

	virtual void init() override {
		m_output = create_output_plug("out", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {}

	void send(int val) {
		m_output->value().append_int(val);
		m_output->fire();
	}

	const ZstURI & output_URI() {
		return m_output->get_URI();
	}
};


class InputComponent : public ZstFilter {
private:
    ZstInputPlug * m_input;

public:
	int num_hits = 0;
	int compare_val = 0;
	int last_received_val = 0;
	bool log = false;

	InputComponent(const char * name, ZstEntityBase * parent, int cmp_val) : compare_val(cmp_val), ZstFilter("TESTER", name, parent) {
		init();
	}

	virtual void init() override {
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		num_hits++;
		last_received_val = plug->value().int_at(0);
		if (log) {
			std::cout << "Input filter received value " << last_received_val << std::endl;
		}
	}

	const ZstURI & input_URI() {
		return m_input->get_URI();
	}

	void reset() {
		num_hits = 0;
	}
};



//Global test variables
ZstStage *stage;
ZstEntityBase * root_entity;

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

	assert(strcmp(uri_equal1.segment(0), "ins") == 0);
	assert(strcmp(uri_equal1.segment(1), "someplug") == 0);
	bool thrown_range_error = false;
	try {
		uri_equal1.segment(2);
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

	root_entity = new ZstEntityBase("ROOT", "root_entity");
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

void test_create_entities(){
	std::cout << "Starting create plugs test" << std::endl;
	int expected_entities = 1;
	int expected_plugs = 1;

	//Attach stage level callback to watch for arriving plugs
	TestPlugArrivingEventCallback * plugArrivalCallback = new TestPlugArrivingEventCallback();
	Showtime::attach_plug_arriving_callback(plugArrivalCallback);

	//Create filters to hold out test plugs
	OutputComponent * test_output = new OutputComponent("entity_create_test_ent", root_entity);
	wait_for_callbacks(expected_entities + expected_plugs);
	assert(plugArrivalCallback->plugArrivedHits == expected_plugs);
	plugArrivalCallback->reset();
	Showtime::remove_plug_arriving_callback(plugArrivalCallback);
	delete plugArrivalCallback;

    //Check stage registered plugs successfully
    assert(stage->get_plug_by_URI(test_output->output_URI()) != NULL);

	//Check local client registered plugs correctly
	ZstURI localPlug_uri = test_output->get_plug_by_URI(test_output->output_URI())->get_URI();
	ZstURI localPlug_uri_via_entity = test_output->output_URI();
	assert(localPlug_uri == localPlug_uri_via_entity);

	//Check plug destruction
	TestPlugLeavingEventCallback * plugLeavingCallback = new TestPlugLeavingEventCallback();
	Showtime::attach_plug_leaving_callback(plugLeavingCallback);

	//Test plug destruction when destroying entity
	ZstURI outURI = test_output->output_URI();
	delete test_output;
	wait_for_callbacks(expected_entities + expected_plugs); 
	assert(stage->get_plug_by_URI(outURI) == NULL);

	//Make sure that the plug leaving callback was hit the correct number of times
	assert(plugLeavingCallback->plugLeavingHits == 1);
	plugLeavingCallback->reset();
	Showtime::remove_plug_leaving_callback(plugLeavingCallback);
	clear_callback_queue();

	std::cout << "Finished create plugs test\n" << std::endl;
}


void test_connect_plugs() {
	std::cout << "Starting connect plugs test" << std::endl;
	
	int expected_entities = 2;
	int expected_plugs = expected_entities;
	int expected_cables = 1;
	OutputComponent * test_output = new OutputComponent("connect_test_ent_out", root_entity);
	InputComponent * test_input = new InputComponent("connect_test_ent_in", root_entity, 0);

	//Test cable callbacks
	TestCableArrivingEventCallback * cableArriveCallback = new TestCableArrivingEventCallback();
	TestCableLeavingEventCallback * cableLeaveCallback = new TestCableLeavingEventCallback();

	Showtime::attach_cable_arriving_callback(cableArriveCallback);
	Showtime::attach_cable_leaving_callback(cableLeaveCallback);
	Showtime::connect_cable(test_output->output_URI(), test_input->input_URI());
	wait_for_callbacks(expected_entities + expected_plugs + expected_cables);
	assert(cableArriveCallback->cableArrivedHits == 1);
	cableArriveCallback->reset();

	ZstURI outURI = test_output->output_URI();
	ZstURI inURI = test_input->input_URI();
	assert(Showtime::endpoint().get_cable_by_URI(outURI, inURI) != NULL);

	//Test connecting missing URI objects
	ZstURI badURI = ZstURI("test_instrument/bad_plug");
	std::cout << " Testing bad cable connection request" << std::endl;
	assert(Showtime::connect_cable(test_output->output_URI(), badURI) <= 0);
	std::cout << "Finished testing bad cable connection request" << std::endl;

	//Testing connection disconnect and reconnect
	assert(Showtime::destroy_cable(test_output->output_URI(), test_input->input_URI()));
	wait_for_callbacks(1);
	assert(cableLeaveCallback->cableLeavingHits == 1);
	cableLeaveCallback->reset();
	assert(Showtime::endpoint().get_cable_by_URI(test_output->output_URI(), test_input->input_URI()) == NULL);

	//Test plug destruction causes cable destruction
	Showtime::connect_cable(test_output->output_URI(), test_input->input_URI());
	delete test_output;
	test_output = 0;
	wait_for_callbacks(expected_cables + 1);
	assert(cableLeaveCallback->cableLeavingHits == 1);
	cableLeaveCallback->reset();
	delete test_input;
	test_input = 0;

	//Test removing callbacks
	Showtime::remove_cable_arriving_callback(cableArriveCallback);
	Showtime::remove_cable_leaving_callback(cableLeaveCallback);
	delete cableArriveCallback;
	delete cableLeaveCallback;
	clear_callback_queue();

	std::cout << "Finished connect plugs test\n" << std::endl;
}


void test_add_filter() {
	std::cout << "Starting addition filter test" << std::endl;
	
	int expected_entities = 4;
	int expected_plugs = 6;
	int expected_cables = 3;
	int first_cmp_val = 4;
	int second_cmp_val = 30;

	//Create a test filter to hold out in/out plugs
	OutputComponent * test_output_augend = new OutputComponent("add_test_augend", root_entity);
	OutputComponent * test_output_addend = new OutputComponent("add_test_addend", root_entity);
	InputComponent * test_input_sum = new InputComponent("add_test_sum", root_entity, first_cmp_val);
	test_input_sum->log = true;
	AddFilter * add_filter = new AddFilter(root_entity);

	Showtime::connect_cable(test_output_augend->output_URI(), add_filter->augend()->get_URI());
	Showtime::connect_cable(test_output_addend->output_URI(), add_filter->addend()->get_URI());
	Showtime::connect_cable(test_input_sum->input_URI(), add_filter->sum()->get_URI());
	wait_for_callbacks(expected_entities + expected_plugs + expected_cables);

	//Send values
	test_output_augend->send(2);
	test_output_addend->send(2);

	//Wait for the first two input callbacks to clear before we check for the sum
	while(test_input_sum->num_hits < 2)
		Showtime::poll_once();
	assert(test_input_sum->last_received_val == first_cmp_val);
	test_input_sum->reset();

	//Send more values
	test_input_sum->compare_val = second_cmp_val;
	test_output_augend->send(20);
	test_output_addend->send(10);

	while (test_input_sum->num_hits < 2)
		Showtime::poll_once();
	assert(test_input_sum->last_received_val == second_cmp_val);

	//Cleanup
	delete test_output_augend;
	delete test_output_addend;
	delete test_input_sum;
	delete add_filter;
	test_output_augend = 0;
	test_output_addend = 0;
	test_input_sum = 0;
	add_filter = 0;
	clear_callback_queue();

	std::cout << "Finished addition filter test" << std::endl;
}


void test_memory_leaks() {
	std::cout << "Starting memory leak test" << std::endl;

	OutputComponent * test_output = new OutputComponent("memleak_test_out", root_entity);
	InputComponent * test_input = new InputComponent("memleak_test_in", root_entity, 10);
	Showtime::connect_cable(test_output->output_URI(), test_input->input_URI());

	int count = 2000;
	std::cout << "Sending " << count << " messages" << std::endl;

	for (int i = 0; i < count; ++i) {
		test_output->send(10);
		Showtime::poll_once();
	}

	delete test_output;
	delete test_input;
	clear_callback_queue();

	std::cout << "Finished memory leak test" << std::endl;
}

void test_leaving(){
    Showtime::leave();
    TAKE_A_BREATH
    assert(stage->get_endpoint_ref_by_UUID(Showtime::endpoint().get_endpoint_UUID()) == NULL);
    TAKE_A_BREATH
}


void test_cleanup() {
	//Test late entity destruction after library cleanup
	delete stage;
	stage = NULL;

	Showtime::destroy();
	delete root_entity;
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
    test_create_entities();
	test_connect_plugs();
	test_add_filter();
	test_memory_leaks();
    test_leaving();
	test_cleanup();
	std::cout << "\nShowtime test successful" << std::endl;

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
