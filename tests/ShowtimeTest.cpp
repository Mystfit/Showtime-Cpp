#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <sstream>
#include <exception>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include "Showtime.h"
#include "ZstPlug.h"
#include "ZstStage.h"
#include "ZstEndpoint.h"
#include "entities/ZstProxyComponent.h"
#include "ZstComposer.h"
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

#define MAX_WAIT 20

inline void wait_for_callbacks(int expected_messages) {
    int repeats = 0;
	while (Showtime::event_queue_size() < expected_messages) {
		TAKE_A_BREATH
        repeats++;
        if(repeats > MAX_WAIT){
            std::ostringstream err;
            err << "Not enough events in queue. Expecting " << expected_messages << " received " << Showtime::event_queue_size() << std::endl;
            throw std::logic_error(err.str());
        }
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
	void run(ZstEntityBase * entity) override {
		std::cout << "ZST_TEST CALLBACK - entity arriving " << entity->URI().path() << std::endl;
		entityArrivedHits++;
		last_created_entity = std::string(entity->URI().path());
	}
	void reset() { entityArrivedHits = 0; }
};

class TestEntityLeavingEventCallback : public ZstEntityEventCallback {
public:
	int entityLeavingHits = 0;
    std::string last_leaving_entity;
	void run(ZstEntityBase * entity) override {
		std::cout << "ZST_TEST CALLBACK - entity leaving " << entity->URI().path() << std::endl;
		entityLeavingHits++;
        last_leaving_entity = std::string(entity->URI().path());
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


//class EntityTemplateArrivingCallback : public ZstEntityTemplateEvent {
//public:
//    int hits = 0;
//    void run(ZstEntityTemplate entity_template) override {
//        std::cout << "ZST_TEST CALLBACK - entity_template arriving Type:" << entity_template.entity_type() << " Owner: " << entity_template.owner() << std::endl;
//        hits++;
//    }
//    void reset() { hits = 0; }
//};

        
// ----



class OutputComponent : public ZstComponent {
private:
    ZstOutputPlug * m_output;

public:
	OutputComponent(const char * name, ZstEntityBase * parent) : ZstComponent("TESTER", name, parent) {
		activate();
		init();
	}

	virtual void init() override {
		m_output = create_output_plug("out", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {}

	void send(int val) {
		m_output->append_int(val);
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

	InputComponent(const char * name, ZstEntityBase * parent, int cmp_val) : ZstFilter("TESTER", name, parent), compare_val(cmp_val) {
		activate();
		init();
	}

	virtual void init() override {
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		num_hits++;
		last_received_val = plug->int_at(0);
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


class AddComposer : public ZstComposer {
public:
    AddComposer() : ZstComposer("add"), adder(NULL) {
        
    }
    
    ~AddComposer(){
        delete adder;
    }
    
    virtual void create(std::string entity_name, ZstEntityBase * parent) override {
        adder = new AddFilter(parent);
    }

    AddFilter * adder;
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

	//Define test URIs
	ZstURI uri_empty = ZstURI();
	ZstURI uri_equal1 = ZstURI("ins/someplug");
	ZstURI uri_notequal = ZstURI("anotherins/someplug");

	//Test accessors
	assert(strcmp(uri_equal1.segment(0), "ins") == 0);
	assert(strcmp(uri_equal1.segment(1), "someplug") == 0);
	assert(uri_empty.is_empty());
	assert(uri_equal1.size() == 2);
	assert(strcmp(uri_equal1.path(), "ins/someplug") == 0);

	//Test comparisons
	assert(ZstURI::equal(uri_equal1, uri_equal1));
	assert(!ZstURI::equal(uri_equal1, uri_notequal));
	assert(ZstURI::equal(ZstURI("root_entity/filter"), ZstURI("root_entity/filter")));
	assert(!(ZstURI::equal(ZstURI("root_entity"), ZstURI("root_entity/filter"))));
	assert(!(ZstURI::equal(ZstURI("root_entity"), ZstURI("root_entity/filter"))));
	assert(ZstURI("b") < ZstURI("c"));
	assert(ZstURI("a") < ZstURI("b"));
	assert(ZstURI("a") < ZstURI("c"));
	assert(!(ZstURI("c") < ZstURI("a")));
	assert(uri_equal1.contains(ZstURI("ins")));
	assert(uri_equal1.contains(ZstURI("ins/someplug")));
	assert(!uri_equal1.contains(ZstURI("nomatch")));

	//Test slicing
	assert(ZstURI::equal(uri_equal1.range(0, 1), ZstURI("ins/someplug")));
	assert(ZstURI::equal(uri_equal1.range(1, 1), ZstURI("someplug")));
	assert(ZstURI::equal(uri_equal1.range(0, 0), ZstURI("ins")));

	//Test joining
	ZstURI joint_uri = ZstURI("a") + ZstURI("b");
	assert(ZstURI::equal(joint_uri, ZstURI("a/b")));
	assert(joint_uri.size() == 2);
	assert(joint_uri.full_size() == 3);

	//Test URI going out of scope
	{
		ZstURI stack_uri("some_entity/some_name");
	}

	//Test range exceptions
	bool thrown_range_error = false;
	try {
		uri_equal1.segment(2);
	}
	catch (std::range_error){
		thrown_range_error = true;
	}
	assert(thrown_range_error);
	thrown_range_error = false;

	try {
		uri_equal1.range(0, 4);
	}
	catch (std::range_error) {
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

void test_startup() {
	stage = ZstStage::create_stage();
	std::cout << "Stage created" << std::endl;

	Showtime::init();
	Showtime::join("127.0.0.1");
}

void test_root_entity() {
	TAKE_A_BREATH
    //Test single entity init
	std::cout << "Starting entity init test" << std::endl;
	Showtime::endpoint().self_test();

	root_entity = new ZstComponent("ROOT", "root_entity");
	root_entity->activate();
	assert(root_entity);
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
	std::cout << "Starting entity test" << std::endl;
	int expected_entities = 1;
	int expected_plugs = 1;

	//Attach stage level callback to watch for arriving plugs
	TestPlugArrivingEventCallback * plugArrivalCallback = new TestPlugArrivingEventCallback();
    Showtime::attach(plugArrivalCallback, ZstCallbackAction::ARRIVING);

	//Create filters to hold out test plugs
	OutputComponent * test_output = new OutputComponent("entity_create_test_ent", root_entity);
	wait_for_callbacks(expected_entities + expected_plugs);
	assert(plugArrivalCallback->plugArrivedHits == expected_plugs);
	plugArrivalCallback->reset();
    Showtime::detach(plugArrivalCallback, ZstCallbackAction::ARRIVING);
	delete plugArrivalCallback;

    //Check stage registered plugs successfully
    assert(stage->get_plug_by_URI(test_output->output_URI()) != NULL);

	//Check local client registered plugs correctly
	ZstURI localPlug_uri = test_output->get_plug_by_URI(test_output->output_URI())->get_URI();
	ZstURI localPlug_uri_via_entity = test_output->output_URI();
	assert(ZstURI::equal(localPlug_uri, localPlug_uri_via_entity));

	//Check plug destruction
	TestPlugLeavingEventCallback * plugLeavingCallback = new TestPlugLeavingEventCallback();
    Showtime::attach(plugLeavingCallback, ZstCallbackAction::LEAVING);

	//Test plug destruction when destroying entity
	ZstURI outURI = test_output->output_URI();
	delete test_output;
	wait_for_callbacks(expected_entities + expected_plugs); 
	assert(stage->get_plug_by_URI(outURI) == NULL);

	//Make sure that the plug leaving callback was hit the correct number of times
	assert(plugLeavingCallback->plugLeavingHits == 1);
	plugLeavingCallback->reset();
    Showtime::detach(plugLeavingCallback, ZstCallbackAction::LEAVING);
	clear_callback_queue();

	std::cout << "Finished entity test\n" << std::endl;
}

//void test_entity_templates(){
//    //Add entity_template listener
//    EntityTemplateArrivingCallback * entity_template_arriving = new EntityTemplateArrivingCallback();
//    Showtime::attach(entity_template_arriving, ZstCallbackAction::ARRIVING);
//
//    //Register a simple composer to create add filters
//    AddComposer * add_composer = new AddComposer();
//    Showtime::register_composer(add_composer);
//    
//    //Test that we recieved a new entity_template locally
//    wait_for_callbacks(1);
//    assert(entity_template_arriving->hits == 1);
//    
//    //Test entity composer
//    Showtime::run_template(add_composer->get_template());
//    
//}

void test_hierarchy() {
	std::cout << "Starting hierarchy test" << std::endl;

	//Test hierarchy
	ZstComponent * parent = new ZstComponent("PARENT", "parent", root_entity);
	ZstComponent * child = new ZstComponent("CHILD", "child", parent);
	parent->activate();
	child->activate();

	wait_for_callbacks(2);
	assert(root_entity->find_child_by_URI(parent->URI()));
	assert(parent->find_child_by_URI(child->URI()));
	assert(Showtime::get_entity_by_URI(parent->URI()));
	assert(Showtime::get_entity_by_URI(child->URI()));

	std::cout << "Removing child..." << std::endl;

	//Test child removal from parent
	ZstURI child_URI = ZstURI(child->URI());
	delete child;
	child = 0;
	wait_for_callbacks(1);
	assert(!parent->find_child_by_URI(child_URI));
	assert(!Showtime::get_entity_by_URI(child_URI));

	//Test removing parent removes child
	std::cout << "Creating new child to test parent removes all children" << std::endl;

	child = new ZstComponent("CHILD", "child", parent);
	child->activate();
	TAKE_A_BREATH

	clear_callback_queue();
	ZstURI parent_URI = ZstURI(parent->URI());
	delete parent;
	wait_for_callbacks(2);
	assert(!root_entity->find_child_by_URI(parent_URI));
	assert(!Showtime::get_entity_by_URI(parent_URI));
	assert(!Showtime::get_entity_by_URI(child_URI));

	delete child;
	child = 0;
	parent = 0;

	std::cout << "Cleanup..." << std::endl;

	clear_callback_queue();

	std::cout << "Finished hierarchy test\n" << std::endl;
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
    Showtime::attach(cableArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::attach(cableLeaveCallback, ZstCallbackAction::LEAVING);
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
    Showtime::detach(cableArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::detach(cableLeaveCallback, ZstCallbackAction::LEAVING);
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

	std::cout << "Finished addition filter test\n" << std::endl;
}


void test_create_proxies(std::string external_sink_path) {
	//Create callbacks
	TestEntityArrivingEventCallback * entityArriveCallback = new TestEntityArrivingEventCallback();
	TestEntityLeavingEventCallback * entityLeaveCallback = new TestEntityLeavingEventCallback();
    Showtime::attach(entityArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::attach(entityLeaveCallback, ZstCallbackAction::LEAVING);

	//Create emitter
	OutputComponent * output = new OutputComponent("proxy_test_output", root_entity);
	TAKE_A_BREATH
		clear_callback_queue();

	//Run sink in external process so we don't share the same Showtime singleton
	std::cout << "Starting sink process" << std::endl;
	std::cout << "----" << std::endl;

	std::string prog = external_sink_path + "/SinkTest";
#ifdef WIN32
	prog += ".exe";
#endif
	boost::process::child sink_process = boost::process::child(prog, "1");

	//Wait for the sink to register its entity and for us to receive the proxy event
	wait_for_callbacks(3);
	assert(entityArriveCallback->entityArrivedHits == 2);
	entityArriveCallback->reset();
	ZstProxyComponent * sink_root = dynamic_cast<ZstProxyComponent*>(Showtime::get_entity_by_URI(ZstURI("sink_root")));
	ZstProxyComponent * sink = dynamic_cast<ZstProxyComponent*>(Showtime::get_entity_by_URI(ZstURI("sink_root/sink")));

	assert(sink_root);
	assert(sink);
	assert(sink_root->is_proxy());
	assert(sink->is_proxy());
	assert(ZstURI::equal(sink->parent()->URI(), sink_root->URI()));
	assert(Showtime::event_queue_size() == 0);

	//Connect cable to sink (it will exit when it receives a message)
	Showtime::connect_cable(output->output_URI(), ZstURI("sink_root/sink/in"));
	TAKE_A_BREATH
	output->send(1);
	sink_process.wait();

	//Check that we received proxy destruction events
	wait_for_callbacks(4);
	assert(entityLeaveCallback->entityLeavingHits == 2);
	assert(Showtime::get_entity_by_URI(ZstURI("sink_root")) == NULL);
	assert(Showtime::get_entity_by_URI(ZstURI("sink_root/sink")) == NULL);

	//Cleanup
    Showtime::detach(entityArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::detach(entityLeaveCallback, ZstCallbackAction::LEAVING);
	delete entityArriveCallback;
	delete entityLeaveCallback;
	sink_root = 0;
	sink = 0;
	clear_callback_queue();

	std::cout << "Finished proxy test\n" << std::endl;
}


void test_memory_leaks(int num_loops) {
	std::cout << "Starting memory leak test" << std::endl;

	OutputComponent * test_output = new OutputComponent("memleak_test_out", root_entity);
	InputComponent * test_input = new InputComponent("memleak_test_in", root_entity, 10);
	Showtime::connect_cable(test_output->output_URI(), test_input->input_URI());
	TAKE_A_BREATH

	int count = num_loops;

	Showtime::endpoint().reset_graph_recv_tripmeter();
	Showtime::endpoint().reset_graph_send_tripmeter();

	std::cout << "Sending " << count << " messages" << std::endl;
	TAKE_A_BREATH


	// Wait until our message tripmeter has received all the messages
	auto delta = std::chrono::milliseconds(-1);
	std::chrono::time_point<std::chrono::system_clock> end, last, now;
	auto start = std::chrono::system_clock::now();
	last = start;
	int last_message_count = 0;
	int message_count = 0;
	int delta_messages = 0;
	long mps = 0.0;
	int remaining_messages = count;
	int queued_messages = 0;
	int delta_queue = 0;
	int last_queue_count = 0;
	long queue_speed = 0;

	for (int i = 0; i < count; ++i) {
		test_output->send(10);
		Showtime::poll_once();
		if (Showtime::endpoint().graph_recv_tripmeter() % 10000 == 0) {
			//Display progress
			message_count = Showtime::endpoint().graph_recv_tripmeter();
			queued_messages = Showtime::endpoint().graph_send_tripmeter() - Showtime::endpoint().graph_recv_tripmeter();

			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			delta_queue = queued_messages - last_queue_count;

			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			queue_speed = (long)delta_queue / (delta.count() / 1000.0);

			remaining_messages = count - message_count;
			last_message_count = message_count;
			last_queue_count = queued_messages;

			std::cout << "Processing " << mps << " messages per/s. Remaining:" << remaining_messages << " Delta time: " << (delta.count() / 1000.0) << " per 10000. Queued messages: " << queued_messages << ". Queuing speed: " << queue_speed << "messages per/s" << std::endl;
		}
	}
	
	std::cout << "Sent all messages. Waiting for recv" << std::endl;

	do  {
		Showtime::poll_once();
		if (Showtime::endpoint().graph_recv_tripmeter() % 10000 == 0) {
			//Display progress
			message_count = Showtime::endpoint().graph_recv_tripmeter();
			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			queued_messages = Showtime::endpoint().graph_send_tripmeter() - Showtime::endpoint().graph_recv_tripmeter();
			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			remaining_messages = count - message_count;
			last_message_count = message_count;

			std::cout << "Processing " << mps << " messages per/s. Remaining:" << remaining_messages << " Delta time: " << (delta.count() / 1000.0) << " per 10000. Queued messages: " << queued_messages << std::endl;
		}
	} while ((Showtime::endpoint().graph_recv_tripmeter() < count));

	TAKE_A_BREATH
	Showtime::poll_once();
	std::cout << "Received all messages" << std::endl;
	std::cout << "Remaining events: " << Showtime::event_queue_size() << std::endl;
	std::cout << "Total received graph_messages " << Showtime::endpoint().graph_recv_tripmeter() << std::endl;
	assert(test_input->num_hits == count);

	delete test_output;
	delete test_input;
	clear_callback_queue();
	Showtime::endpoint().reset_graph_recv_tripmeter();
	Showtime::endpoint().reset_graph_send_tripmeter();

	std::cout << "Finished memory leak test\n" << std::endl;
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
	test_standard_layout();
	test_URI();
	test_startup();
	test_root_entity();
    test_stage_registration();
    test_create_entities();
//    test_entity_templates();
	test_hierarchy();
	test_connect_plugs();
	test_add_filter();
	test_create_proxies(boost::filesystem::system_complete(argv[0]).parent_path().generic_string());
	test_memory_leaks(200000);
    test_leaving();
	test_cleanup();
	std::cout << "\nShowtime test successful" << std::endl;

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
