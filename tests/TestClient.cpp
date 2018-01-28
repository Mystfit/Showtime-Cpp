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

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 200);
#endif

#define MAX_WAIT 20
void wait_for_event(ZstEvent * callback, int expected_messages)
{
	int repeats = 0;
	zst_poll_once();
	while (callback->num_calls() < expected_messages) {
		TAKE_A_BREATH
		repeats++;
		if (repeats > MAX_WAIT) {
			std::ostringstream err;
			err << "Not enough events in queue. Expecting " << expected_messages << " received " << callback->num_calls() << std::endl;
			throw std::runtime_error(err.str());
		}
		zst_poll_once();
	}
}

inline void clear_callback_queue() {
	zst_poll_once();
}

//Callback classes
//----------------
class TestEntityEventCallback : public ZstComponentEvent {
public:
	std::string last_entity;
	std::string m_suffix;
	TestEntityEventCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstComponent * component) override {
		LOGGER->debug("ENTITY_EVENT: {} {}", component->URI().path(), m_suffix);
		last_entity = std::string(component->URI().path());
	}
};

class TestConnectCallback : public ZstPerformerEvent {
public:
	std::string m_suffix;
	TestConnectCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstPerformer * root_performer) override {
		LOGGER->debug("CONN_EVENT: {} {}", root_performer->URI().path(), m_suffix);
	}
};

class TestPlugEventCallback : public ZstPlugEvent {
public:
	std::string m_suffix;
	TestPlugEventCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstPlug * plug) override {
		LOGGER->debug("PLUG_EVENT: {} {}", plug->URI().path(), m_suffix);
	}
};

class TestCableEventCallback : public ZstCableEvent {
public:
	std::string m_suffix;
	TestCableEventCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstCable * cable) override {
		LOGGER->debug("CABLE_EVENT: {} {} {}", cable->get_output_URI().path(), cable->get_input_URI().path(), m_suffix);
	}
};

class TestEntityTemplateCallback : public ZstComponentTypeEvent {
public:
	std::string m_suffix;
	TestEntityTemplateCallback(std::string suffix) {
		m_suffix = suffix;
	}
    void run(ZstComponent * component_template) override {
		LOGGER->debug("TEMPLATE_EVENT: {} Owner {}", component_template->entity_type(), component_template->parent()->URI().path(), m_suffix);
    }
};

class TestPerformerCallback : public ZstPerformerEvent {
public:
	std::string m_suffix;
	TestPerformerCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstPerformer * performer) override {
		LOGGER->debug("PERFORMER_EVENT: {} {}", performer->URI().path(), m_suffix);
	}
};

        
// ----



class OutputComponent : public ZstComponent {
private:
    ZstOutputPlug * m_output;

public:
	OutputComponent(const char * name) : ZstComponent("TESTER", name) {
		m_output = create_output_plug("out", ZstValueType::ZST_FLOAT);
	}

	virtual void compute(ZstInputPlug * plug) override {}

	void send(int val) {
		m_output->append_float(val);
		m_output->fire();
	}

	ZstPlug * output() {
		return m_output;
	}
};


class InputComponent : public ZstComponent {
private:
    ZstInputPlug * m_input;

public:
	int num_hits = 0;
	int compare_val = 0;
	int last_received_val = 0;
	bool log = false;

	InputComponent(const char * name, int cmp_val, bool should_log=false) : 
		ZstComponent("TESTER", name), compare_val(cmp_val)
	{
		log = should_log;
		m_input = create_input_plug("in", ZstValueType::ZST_FLOAT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		num_hits++;
		last_received_val = plug->float_at(0);
		if (log) {
			LOGGER->debug("Input filter received value {0:d}", last_received_val);
		}
	}

	ZstPlug * input() {
		return m_input;
	}

	void reset() {
		num_hits = 0;
	}
};


void test_standard_layout() {
	//Verify standard layout
	assert(std::is_standard_layout<ZstURI>());
}

void test_URI() {
	LOGGER->info("Running URI test");

	//Define test URIs
	ZstURI uri_empty = ZstURI();
	ZstURI uri_single = ZstURI("single");
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
	assert(ZstURI::equal(uri_equal1.parent(), ZstURI("ins")));
	assert(ZstURI::equal(uri_equal1.first(), ZstURI("ins")));

	bool thrown_URI_range_error = false;
	try {
		uri_single.parent();
	}
	catch (std::out_of_range) {
		thrown_URI_range_error = true;
	}
	assert(thrown_URI_range_error);
	thrown_URI_range_error = false;

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
}

void test_startup() {

	LOGGER->info("Running Showtime init test");

	//Test connection
	TestConnectCallback * connectCallback = new TestConnectCallback("connected");
	zst_attach_connection_event_listener(connectCallback);

	zst_init("TestClient");
	zst_join("127.0.0.1");

	wait_for_event(connectCallback, 1);
	assert(connectCallback->num_calls() == 1);
	assert(zst_is_connected());
	LOGGER->debug("Connection successful");
	
	zst_remove_connection_event_listener(connectCallback);
	delete connectCallback;

	//assert(zst_ping() >= 0);
}

void test_root_entity() {
	LOGGER->info("Running entity init test");

	//Test root entity is activated
    ZstPerformer * root_entity = zst_get_root();
	assert(root_entity);
    assert(root_entity->is_activated());
	clear_callback_queue();
	LOGGER->debug("Root entity is activated");
}


void test_create_entities(){
	LOGGER->info("Running entity test");
	int expected_entities = 1;
	int expected_plugs = expected_entities * 1;
	
	//Create entities
	LOGGER->debug("Creating entities and events");
	OutputComponent * test_output = new OutputComponent("entity_create_test_ent");
	TestEntityEventCallback * entity_activated_local = new TestEntityEventCallback("activated via local event");
	TestEntityEventCallback * entity_deactivated_local = new TestEntityEventCallback("deactivated via local event");
	test_output->attach_activation_event(entity_activated_local);
	test_output->attach_deactivation_event(entity_deactivated_local);
	
	zst_activate_entity(test_output);
	wait_for_event(entity_activated_local, 1);
	assert(entity_activated_local->num_calls() == 1);
	entity_activated_local ->reset_num_calls();
    
	//Check local client registered plugs correctly
	LOGGER->debug("Verify entity is activated");
	assert(test_output->is_activated());
	ZstURI localPlug_uri = test_output->get_plug_by_URI(test_output->output()->URI())->URI();
	ZstURI localPlug_uri_via_entity = test_output->output()->URI();
	assert(ZstURI::equal(localPlug_uri, localPlug_uri_via_entity));

	//Cleanup
	LOGGER->debug("Deactivate entity");
	zst_deactivate_entity(test_output);
	wait_for_event(entity_deactivated_local, 1);
	assert(entity_deactivated_local->num_calls() == 1);
	entity_deactivated_local->reset_num_calls();
	assert(!test_output->is_activated());
	assert(!zst_find_entity(test_output->URI()));
	LOGGER->debug("Entity successfully deactivated");

	delete test_output;
	delete entity_activated_local;
	
	assert(!zst_get_root()->find_child_by_URI(localPlug_uri));
}


void test_hierarchy() {
	LOGGER->info("Running hierarchy test");

	//Test hierarchy
	ZstContainer * parent = new ZstContainer("parent");
	ZstComponent * child = new ZstComponent("child");
	parent->add_child(child);

	TestEntityEventCallback * parent_activated = new TestEntityEventCallback("parent activated");
	TestEntityEventCallback * parent_deactivated = new TestEntityEventCallback("parent deactivated");
	TestEntityEventCallback * child_activated = new TestEntityEventCallback("child activated");
	TestEntityEventCallback * child_deactivated = new TestEntityEventCallback("child deactivated");

	parent->attach_activation_event(parent_activated);
	parent->attach_deactivation_event(parent_deactivated);
	child->attach_activation_event(child_activated);
	child->attach_deactivation_event(child_deactivated);

	zst_activate_entity(parent);
	wait_for_event(parent_activated, 1);
	assert(parent_activated->num_calls() == 1);
	parent_activated->reset_num_calls();
		
	assert(zst_find_entity(parent->URI()));
	assert(zst_find_entity(child->URI()));
	
	//Test child removal from parent
	LOGGER->debug("Testing child removal from parent");
	ZstURI child_URI = ZstURI(child->URI());
	zst_deactivate_entity(child);
	wait_for_event(child_deactivated, 1);
	assert(child_deactivated->num_calls() == 1);
	child_deactivated->reset_num_calls();
	delete child;
	child = 0;
	assert(!parent->find_child_by_URI(child_URI));
	assert(!zst_get_root()->find_child_by_URI(child_URI));

	//Test removing parent removes child
	LOGGER->debug("Creating new child to test if parent removes all children");
	child = new ZstComponent("child");
	parent->add_child(child);
	child->attach_activation_event(child_activated);
	zst_activate_entity(child);
	wait_for_event(child_activated, 1);
	child_activated->reset_num_calls();
	
	ZstURI parent_URI = ZstURI(parent->URI());
	zst_deactivate_entity(parent);
	wait_for_event(parent_deactivated, 1);
	assert(parent_deactivated->num_calls() == 1);
	parent_deactivated->reset_num_calls();
	assert(!zst_get_root()->find_child_by_URI(parent_URI));
	assert(!zst_get_root()->find_child_by_URI(child_URI));
	delete parent;
	child = 0;
	parent = 0;

	delete parent_activated;
	delete parent_deactivated;
	delete child_activated;
	delete child_deactivated;
	clear_callback_queue();
}


void test_connect_plugs() {
	LOGGER->info("Running connect plugs test");
	
	int expected_entities = 2;
	int expected_plugs = expected_entities;
	int expected_cables = 1;

	LOGGER->debug("Creating components to test cable connections");
	OutputComponent * test_output = new OutputComponent("connect_test_ent_out");
	InputComponent * test_input = new InputComponent("connect_test_ent_in", 0);
	TestEntityEventCallback * entity_activated = new TestEntityEventCallback("activated");
	TestCableEventCallback * cable_activated_local = new TestCableEventCallback("activated via stage update");
	TestCableEventCallback * cable_deactivated_local = new TestCableEventCallback("cable deactivated via local event");
	
	LOGGER->debug("Attaching events");
	test_output->attach_activation_event(entity_activated);
	test_input->attach_activation_event(entity_activated);
	zst_activate_entity(test_output);
	zst_activate_entity(test_input);
	wait_for_event(entity_activated, expected_entities);
	entity_activated->reset_num_calls();

	LOGGER->debug("Testing cable connection");
	ZstCable * cable = zst_connect_cable(test_output->output(), test_input->input());
	cable->attach_activation_event(cable_activated_local);
	wait_for_event(cable_activated_local, 1);
	assert(cable_activated_local->num_calls() == 1);
	cable_activated_local->reset_num_calls();
	
	LOGGER->debug("Verifying cable");
	assert(test_output->output()->num_cables() == 1);
	assert(cable->get_output() == test_output->output());
	assert(cable->get_input() == test_input->input());
	assert(test_output->output()->is_connected_to(test_input->input()));
	for (auto c : *(test_output->output())) {
		assert(c->get_input() == test_input->input());
	}

	LOGGER->debug("Testing cable disconnection");
	cable->attach_deactivation_event(cable_deactivated_local);
	zst_destroy_cable(cable);
	wait_for_event(cable_deactivated_local, 1);
	assert(cable_deactivated_local->num_calls() == 1);
	cable_deactivated_local->reset_num_calls();
	assert(!test_output->output()->is_connected_to(test_input->input()));
	assert(test_output->output()->num_cables() == 0);
	assert(test_input->input()->num_cables() == 0);

	LOGGER->debug("Testing cable disconnection when removing parent");
	cable = zst_connect_cable(test_output->output(), test_input->input());
	cable->attach_activation_event(cable_activated_local);
	cable->attach_deactivation_event(cable_deactivated_local);
	wait_for_event(cable_activated_local, 1);
	zst_deactivate_entity(test_output);
	wait_for_event(cable_deactivated_local, 1);

	assert(cable_deactivated_local->num_calls() == 1);
	assert(!test_input->input()->is_connected_to(test_output->output()));
	cable_deactivated_local->reset_num_calls();

	//Cleanup
	zst_deactivate_entity(test_output);
	zst_deactivate_entity(test_input);
	clear_callback_queue();
	delete test_output;
	delete test_input;
	delete cable_activated_local;
	delete cable_deactivated_local;
	delete entity_activated;
}


void test_add_filter() {
	LOGGER->info("Starting addition filter test");
		
	int expected_entities = 4;
	int expected_plugs = 6;
	int expected_cables = 3;
	int first_cmp_val = 4;
	int second_cmp_val = 30;

	LOGGER->debug("Creating input/output components for addition filter");
	TestEntityEventCallback * entity_activated = new TestEntityEventCallback("activated");
	TestEntityEventCallback * entity_deactivated = new TestEntityEventCallback("deactivated");
	TestCableEventCallback * cable_event = new TestCableEventCallback("arriving");

	OutputComponent * test_output_augend = new OutputComponent("add_test_augend");
	OutputComponent * test_output_addend = new OutputComponent("add_test_addend");
	InputComponent * test_input_sum = new InputComponent("add_test_sum", first_cmp_val, true);
	AddFilter * add_filter = new AddFilter("add_test");
	
	LOGGER->debug("Attaching events");
	test_output_addend->attach_activation_event(entity_activated);
	test_output_addend->attach_deactivation_event(entity_deactivated);
	test_output_augend->attach_activation_event(entity_activated);
	test_output_augend->attach_deactivation_event(entity_deactivated);
	test_input_sum->attach_activation_event(entity_activated);
	test_input_sum->attach_deactivation_event(entity_deactivated);
	add_filter->attach_activation_event(entity_activated);
	add_filter->attach_deactivation_event(entity_deactivated);
		
	LOGGER->debug("Activating entities");
	zst_activate_entity(test_output_augend);
	zst_activate_entity(test_output_addend);
	zst_activate_entity(test_input_sum);
	zst_activate_entity(add_filter);

	wait_for_event(entity_activated, 4);
	assert(entity_activated->num_calls() == 4);
	entity_activated->reset_num_calls();

	LOGGER->debug("Connecting cables");
	ZstCable * augend_cable = zst_connect_cable(test_output_augend->output(), add_filter->augend());
	augend_cable->attach_activation_event(cable_event);
	ZstCable * addend_cable = zst_connect_cable(test_output_addend->output(), add_filter->addend());
	addend_cable->attach_activation_event(cable_event);
	ZstCable * sum_cable = zst_connect_cable(test_input_sum->input(), add_filter->sum());
	sum_cable->attach_activation_event(cable_event);

	wait_for_event(cable_event, 3);
	cable_event->reset_num_calls();

	//Send values
	LOGGER->debug("Sending values");
	test_output_augend->send(2);
	test_output_addend->send(2);

	TAKE_A_BREATH

	int max_wait = 1000;
	int current_wait = 0;

	//Wait for the first two input callbacks to clear before we check for the sum
    while(test_input_sum->num_hits < 2 && ++current_wait < max_wait){
		zst_poll_once();
    }
	assert(test_input_sum->last_received_val == first_cmp_val);
	test_input_sum->reset();

	//Send more values
	test_input_sum->compare_val = second_cmp_val;
	test_output_augend->send(20);
	test_output_addend->send(10);

	while (test_input_sum->num_hits < 2)
		zst_poll_once();
	assert(test_input_sum->last_received_val == second_cmp_val);
	LOGGER->debug("Addition component succeeded at addition!");

	//Cleanup
	LOGGER->debug("Cleaning up entities");
	zst_deactivate_entity(test_output_augend);
	zst_deactivate_entity(test_output_addend);
	zst_deactivate_entity(test_input_sum);
	zst_deactivate_entity(add_filter);
	wait_for_event(entity_deactivated, 4);
	assert(entity_deactivated->num_calls() == 4);
	clear_callback_queue();

	delete test_output_augend;
	delete test_output_addend;
	delete test_input_sum;
	delete add_filter;
	delete cable_event;
	test_output_augend = 0;
	test_output_addend = 0;
	test_input_sum = 0;
	add_filter = 0;
}


void test_external_entities(std::string external_test_path) {
	LOGGER->info("Starting external entities test");

	//Create callbacks
	TestEntityEventCallback * entityArriveCallback = new TestEntityEventCallback("arriving");
	TestEntityEventCallback * entityLeaveCallback = new TestEntityEventCallback("leaving");
	TestPerformerCallback * performerArriveCallback = new TestPerformerCallback("arriving");
	TestPerformerCallback * performerLeaveCallback = new TestPerformerCallback("leaving");

    zst_attach_component_event_listener(entityArriveCallback, ZstEventAction::ARRIVING);
    zst_attach_component_event_listener(entityLeaveCallback, ZstEventAction::LEAVING);
	zst_attach_performer_event_listener(performerArriveCallback, ZstEventAction::ARRIVING);
	zst_attach_performer_event_listener(performerLeaveCallback, ZstEventAction::LEAVING);

	//Create emitter
	OutputComponent * output = new OutputComponent("proxy_test_output");
	zst_activate_entity(output);
	TAKE_A_BREATH
	clear_callback_queue();

	//Run sink in external process so we don't share the same Showtime singleton
	LOGGER->debug("Starting sink process");
	
	ZstURI sink_perf_uri = ZstURI("sink");
	ZstURI sink_ent_uri = sink_perf_uri + ZstURI("sink_ent");
	ZstURI sink_B_uri = sink_perf_uri + ZstURI("sinkB");
	ZstURI sink_plug_uri = sink_ent_uri + ZstURI("in");

	//Run the sink program
	std::string prog = external_test_path + "/TestSink";
#ifdef WIN32
	prog += ".exe";
#endif
	boost::process::child sink_process;

	try {
		sink_process = boost::process::child(prog, "1");
#ifdef WIN32
		system("pause");
#endif
	}
	catch (boost::process::process_error e) {
		LOGGER->error("Sink process failed to start. Code:{} Message:{}", e.code().value(), e.what());
	}
	assert(sink_process.valid());

	//Test performer arriving
	wait_for_event(performerArriveCallback, 1);
	ZstPerformer * sink_performer = zst_get_performer_by_URI(sink_perf_uri);
	assert(sink_performer);
	
	//Test entity exists
	ZstContainer * sink_ent = dynamic_cast<ZstContainer*>(sink_performer->find_child_by_URI(sink_ent_uri));
	assert(sink_ent);

	ZstPlug * sink_plug = sink_ent->get_plug_by_URI(sink_plug_uri);
	assert(sink_plug);
	assert(sink_plug->is_activated());

	//Connect cable to sink
	TestCableEventCallback * cableArriveCallback = new TestCableEventCallback("arriving");
	TestCableEventCallback * cableLeaveCallback = new TestCableEventCallback("leaving");
	zst_attach_cable_event_listener(cableArriveCallback, ZstEventAction::ARRIVING);
	zst_attach_cable_event_listener(cableLeaveCallback, ZstEventAction::LEAVING);
	ZstCable * cable = zst_connect_cable(output->output(), sink_plug);
	assert(cable);
	cable->attach_activation_event(cableArriveCallback);
	wait_for_event(cableArriveCallback, 1);
	TAKE_A_BREATH
	output->send(1);

	//Test entity arriving
	wait_for_event(entityArriveCallback, 1);
	assert(entityArriveCallback->last_entity == std::string(sink_B_uri.path()));
	assert(zst_find_entity(sink_B_uri));
	entityArriveCallback->reset_num_calls();

	//Send another value to remove the child
	//Test entity leaving
	output->send(2);
	wait_for_event(entityLeaveCallback, 1);
	assert(entityLeaveCallback->last_entity == std::string(sink_B_uri.path()));
	assert(!zst_find_entity(sink_B_uri));
	entityArriveCallback->reset_num_calls();

	output->send(0);
	sink_process.wait();

	//Check that we received performer destruction request
	wait_for_event(performerLeaveCallback, 1);
	assert(!zst_get_performer_by_URI(sink_perf_uri));

	//Cleanup
    zst_remove_component_event_listener(entityArriveCallback, ZstEventAction::ARRIVING);
    zst_remove_component_event_listener(entityLeaveCallback, ZstEventAction::LEAVING);
	zst_remove_performer_event_listener(performerArriveCallback, ZstEventAction::ARRIVING);
	zst_remove_performer_event_listener(performerLeaveCallback, ZstEventAction::LEAVING);
	delete entityArriveCallback;
	delete entityLeaveCallback;
	delete performerArriveCallback;
	delete performerLeaveCallback;
	clear_callback_queue();
}


void test_memory_leaks(int num_loops) {
	LOGGER->info("Starting memory leak test");

	OutputComponent * test_output = new OutputComponent("memleak_test_out");
	InputComponent * test_input = new InputComponent("memleak_test_in", 10);
	zst_activate_entity(test_output);
	zst_activate_entity(test_input);
	zst_connect_cable(test_output->output(), test_input->input());
	TAKE_A_BREATH

	int count = num_loops;

	//ZstClient::instance().reset_graph_recv_tripmeter();
	//ZstClient::instance().reset_graph_send_tripmeter();

	LOGGER->debug("Sending {} messages", count);

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
		zst_poll_once();
		//if (ZstClient::instance().graph_recv_tripmeter() % 10000 == 0) {
		//	//Display progress
		//	message_count = ZstClient::instance().graph_recv_tripmeter();
		//	queued_messages = ZstClient::instance().graph_send_tripmeter() - ZstClient::instance().graph_recv_tripmeter();

		//	now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
		//	delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
		//	delta_messages = message_count - last_message_count;
		//	delta_queue = queued_messages - last_queue_count;

		//	last = now;
		//	mps = (long)delta_messages / (delta.count() / 1000.0);
		//	queue_speed = (long)delta_queue / (delta.count() / 1000.0);

		//	remaining_messages = count - message_count;
		//	last_message_count = message_count;
		//	last_queue_count = queued_messages;

		//	std::cout << "Processing " << mps << " messages per/s. Remaining:" << remaining_messages << " Delta time: " << (delta.count() / 1000.0) << " per 10000. Queued messages: " << queued_messages << ". Queuing speed: " << queue_speed << "messages per/s" << std::endl;
		//}
	}
	
	LOGGER->debug("Sent all messages. Waiting for recv");

	//do  {
	//	zst_poll_once();
	//	if (ZstClient::instance().graph_recv_tripmeter() % 10000 == 0) {
	//		//Display progress
	//		message_count = ZstClient::instance().graph_recv_tripmeter();
	//		now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	//		delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
	//		delta_messages = message_count - last_message_count;
	//		queued_messages = ZstClient::instance().graph_send_tripmeter() - ZstClient::instance().graph_recv_tripmeter();
	//		last = now;
	//		mps = (long)delta_messages / (delta.count() / 1000.0);
	//		remaining_messages = count - message_count;
	//		last_message_count = message_count;

	//		std::cout << "Processing " << mps << " messages per/s. Remaining:" << remaining_messages << " Delta time: " << (delta.count() / 1000.0) << " per 10000. Queued messages: " << queued_messages << std::endl;
	//	}
	//} while ((ZstClient::instance().graph_recv_tripmeter() < count));

	TAKE_A_BREATH
	zst_poll_once();
	LOGGER->debug("Received all messages");
	/*std::cout << "Remaining events: " << zst_event_queue_size() << std::endl;
	std::cout << "Total received graph_messages " << ZstClient::instance().graph_recv_tripmeter() << std::endl;*/
	assert(test_input->num_hits == count);

	delete test_output;
	delete test_input;
	clear_callback_queue();
	/*ZstClient::instance().reset_graph_recv_tripmeter();
	ZstClient::instance().reset_graph_send_tripmeter();*/
}

void test_leaving(){
    zst_leave();
}


void test_cleanup() {
	//Test late entity destruction after library cleanup
	zst_destroy();
}

int main(int argc,char **argv){
	//Give the server time to start (if launching both simultaneously)
	TAKE_A_BREATH
	zst_log_init();
	LOGGER->set_level(spdlog::level::debug);

	std::string ext_test_folder = boost::filesystem::system_complete(argv[0]).parent_path().generic_string();

	//Tests
	test_standard_layout();
	test_URI();
	test_startup();
	test_root_entity();
    test_create_entities();
	test_hierarchy();
	test_connect_plugs();
	test_add_filter();
	test_external_entities(ext_test_folder);
	test_memory_leaks(200000);
    test_leaving();
	test_cleanup();
	LOGGER->info("All tests completed");

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
