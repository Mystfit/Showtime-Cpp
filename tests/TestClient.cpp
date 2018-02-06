#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <sstream>
#include <exception>
#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>

#include "Showtime.h"

#define BOOST_THREAD_DONT_USE_DATETIME
#define MEM_LEAK_LOOPS 200000;

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 200);
#endif

#define MAX_WAIT 2000
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
		ZstLog::debug("ENTITY_EVENT: {} {}", component->URI().path(), m_suffix);
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
		ZstLog::debug("CONN_EVENT: {} {}", root_performer->URI().path(), m_suffix);
	}
};

class TestPlugEventCallback : public ZstPlugEvent {
public:
	std::string m_suffix;
	TestPlugEventCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstPlug * plug) override {
		ZstLog::debug("PLUG_EVENT: {} {}", plug->URI().path(), m_suffix);
	}
};

class TestCableEventCallback : public ZstCableEvent {
public:
	std::string m_suffix;
	TestCableEventCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstCable * cable) override {
		ZstLog::debug("CABLE_EVENT: {} {} {}", cable->get_output_URI().path(), cable->get_input_URI().path(), m_suffix);
	}
};

class TestEntityTemplateCallback : public ZstComponentTypeEvent {
public:
	std::string m_suffix;
	TestEntityTemplateCallback(std::string suffix) {
		m_suffix = suffix;
	}
    void run(ZstComponent * component_template) override {
		ZstLog::debug("TEMPLATE_EVENT: {} Owner {}", component_template->entity_type(), component_template->parent()->URI().path(), m_suffix);
    }
};

class TestPerformerCallback : public ZstPerformerEvent {
public:
	std::string m_suffix;
	TestPerformerCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstPerformer * performer) override {
		ZstLog::debug("PERFORMER_EVENT: {} {}", performer->URI().path(), m_suffix);
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

	void send(float val) {
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
		float actual_val = plug->float_at(0);
		last_received_val = int(actual_val);
		if (log) {
			ZstLog::debug("Input filter received value {0:d}", last_received_val);
		}
	}

	ZstPlug * input() {
		return m_input;
	}

	void reset() {
		num_hits = 0;
	}
};


void test_startup() {
	zst_init("TestClient", true);
	zst_start_file_logging();
	ZstLog::info("Running Showtime init test");

	ZstLog::debug("Testing sync join");
	zst_join("127.0.0.1");
	assert(zst_is_connected());

	//Leave the stage so we can reconnect
	zst_leave();
	assert(!zst_is_connected());

	//Create events to listen for a successful connection
	TestConnectCallback * connectCallback = new TestConnectCallback("connected");
	zst_attach_connection_event_listener(connectCallback);

	ZstLog::debug("Testing async join");
	assert(connectCallback->num_calls() == 0);
	zst_join_async("127.0.0.1");
	wait_for_event(connectCallback, 1);
	assert(connectCallback->num_calls() == 1);
	assert(zst_is_connected());

	//Cleanup
	zst_remove_connection_event_listener(connectCallback);
	delete connectCallback;
}

void test_URI() {
	ZstLog::info("Running URI test");

	assert(std::is_standard_layout<ZstURI>());

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

	//Test cables
	ZstURI in = ZstURI("a/1");
	ZstURI out = ZstURI("b/1");

	ZstCable cable_a = ZstCable(in, out);
	assert(cable_a.get_input_URI() == in);
	assert(cable_a.get_output_URI() == out);
	assert(cable_a.is_attached(out));
	assert(cable_a.is_attached(in));
	
	ZstCable cable_b = ZstCable(in, out);
	assert(cable_b == cable_a);
	assert(ZstCableEq{}(&cable_a, &cable_b));
	assert(ZstCableHash{}(&cable_a) == ZstCableHash{}(&cable_b));

	//Test cable going out of scope
	{
		ZstCable cable_c = ZstCable(ZstURI("foo"), ZstURI("bar"));
		assert(cable_c != cable_a);
	}
}

void test_root_entity() {
	ZstLog::info("Running performer test");

	//Test root entity is activated
	TestPerformerCallback * performer_activated = new TestPerformerCallback("performer activating");
    ZstPerformer * root_entity = zst_get_root();
	assert(root_entity);
    
    //This should execute immediately since we've already connected to the stage
	root_entity->attach_activation_event(performer_activated);
	assert(performer_activated->num_calls() == 1);
	performer_activated->reset_num_calls();
    assert(root_entity->is_activated());
	clear_callback_queue();
	root_entity->detach_activation_event(performer_activated);
	delete performer_activated;
	ZstLog::debug("Root performer is activated");
}


void test_create_entities(){
	ZstLog::info("Running entity creation test");
    
    ZstLog::debug("Creating sync entity");
    OutputComponent * test_output_sync = new OutputComponent("entity_create_test_sync");
    
    ZstLog::debug("Testing entity sync activation");
    zst_activate_entity(test_output_sync);
    assert(test_output_sync->is_activated());
    assert(zst_find_entity(test_output_sync->URI()));
    
    ZstLog::debug("Testing entity sync deactivation");
    zst_deactivate_entity(test_output_sync);
    assert(!test_output_sync->is_activated());
    assert(!zst_find_entity(test_output_sync->URI()));
    delete test_output_sync;
    
    //Test async entity
	OutputComponent * test_output_async = new OutputComponent("entity_create_test_async");
	TestEntityEventCallback * entity_activated_local = new TestEntityEventCallback("activated via local event");
	TestEntityEventCallback * entity_deactivated_local = new TestEntityEventCallback("deactivated via local event");
	test_output_async->attach_activation_event(entity_activated_local);
	test_output_async->attach_deactivation_event(entity_deactivated_local);
	
    ZstLog::debug("Testing entity async activation");
	zst_activate_entity_async(test_output_async);
	wait_for_event(entity_activated_local, 1);
	assert(entity_activated_local->num_calls() == 1);
	entity_activated_local ->reset_num_calls();
    
	//Check local client registered plugs correctly
	assert(test_output_async->is_activated());
	ZstURI localPlug_uri = test_output_async->get_plug_by_URI(test_output_async->output()->URI())->URI();
	ZstURI localPlug_uri_via_entity = test_output_async->output()->URI();
	assert(ZstURI::equal(localPlug_uri, localPlug_uri_via_entity));

    ZstLog::debug("Testing entity async deactivation");
	zst_deactivate_entity(test_output_async);
	wait_for_event(entity_deactivated_local, 1);
	assert(entity_deactivated_local->num_calls() == 1);
	entity_deactivated_local->reset_num_calls();
	assert(!test_output_async->is_activated());
	assert(!zst_find_entity(test_output_async->URI()));
    assert(!zst_find_entity(localPlug_uri));
    
    //Cleanup
	delete test_output_async;
	delete entity_activated_local;
}


void test_hierarchy() {
	ZstLog::info("Running hierarchy test");

	//Test hierarchy
	ZstContainer * parent = new ZstContainer("parent");
	ZstComponent * child = new ZstComponent("child");
	parent->add_child(child);

	zst_activate_entity(parent);
	assert(zst_find_entity(parent->URI()));
	assert(zst_find_entity(child->URI()));
	
	//Test child removal from parent
	ZstLog::debug("Testing child removal from parent");
	ZstURI child_URI = ZstURI(child->URI());
	zst_deactivate_entity(child);
	assert(!parent->find_child_by_URI(child_URI));
	assert(!zst_find_entity(child_URI));
    
    //Test child activation and deactivation callbacks
    TestEntityEventCallback * child_activated = new TestEntityEventCallback("child activated");
    TestEntityEventCallback * child_deactivated = new TestEntityEventCallback("child deactivated");
    child->attach_activation_event(child_activated);
    child->attach_deactivation_event(child_deactivated);
    parent->add_child(child);
    
    zst_activate_entity(child);
    assert(child_activated->num_calls() == 1);
    zst_deactivate_entity(child);
    assert(child_deactivated->num_calls() == 1);
    
    child->detach_activation_event(child_activated);
    child->detach_deactivation_event(child_deactivated);
    delete child_activated;
    delete child_deactivated;
    
	//Test removing parent removes child
	parent->add_child(child);
	zst_activate_entity(child);
    
	ZstURI parent_URI = ZstURI(parent->URI());
	zst_deactivate_entity(parent);
	assert(!zst_find_entity(parent->URI()));
	assert(!zst_find_entity(child->URI()));
	delete parent;
    parent = 0;
	child = 0;

	clear_callback_queue();
}


void test_connect_plugs() {
	ZstLog::info("Running connect plugs test");
	
	ZstLog::debug("Creating entities");
	OutputComponent * test_output = new OutputComponent("connect_test_out");
	InputComponent * test_input = new InputComponent("connect_test_in", 0);
    zst_activate_entity(test_output);
	zst_activate_entity(test_input);

	ZstLog::debug("Testing sync cable connection");
	ZstCable * cable = zst_connect_cable(test_input->input(), test_output->output());
    assert(cable->is_activated());
	
	ZstLog::debug("Verifying cable");
	assert(cable->get_output() == test_output->output());
	assert(cable->get_input() == test_input->input());
    assert(test_output->output()->is_connected_to(test_input->input()));
    assert(test_input->input()->is_connected_to(test_output->output()));
	for (auto c : *(test_output->output())) {
		assert(c->get_input() == test_input->input());
	}

	ZstLog::debug("Testing cable disconnection");
	zst_destroy_cable(cable);
    cable = 0;
    assert(!test_output->output()->is_connected_to(test_input->input()));
    assert(!test_input->input()->is_connected_to(test_output->output()));
    
    ZstLog::debug("Testing async cable connection");
    TestCableEventCallback * cable_activated_local = new TestCableEventCallback("activated via local update");
    TestCableEventCallback * cable_deactivated_local = new TestCableEventCallback("cable deactivated via local event");
    cable = zst_connect_cable_async(test_input->input(), test_output->output());
    cable->attach_activation_event(cable_activated_local);
    wait_for_event(cable_activated_local, 1);
    assert(cable_activated_local->num_calls() == 1);
    cable_activated_local->reset_num_calls();
    
    cable->attach_deactivation_event(cable_deactivated_local);
    zst_destroy_cable_async(cable);
    cable = 0;
    wait_for_event(cable_deactivated_local, 1);
    assert(cable_deactivated_local->num_calls() == 1);
    cable_deactivated_local->reset_num_calls();

	ZstLog::debug("Testing cable disconnection when removing parent");
	cable = zst_connect_cable(test_input->input(), test_output->output());
    cable->attach_deactivation_event(cable_deactivated_local);
    zst_deactivate_entity(test_output);
    wait_for_event(cable_deactivated_local, 1);
	assert(!test_input->input()->is_connected_to(test_output->output()));
    assert(!test_output->output()->is_connected_to(test_input->input()));
    
	//Cleanup
	zst_deactivate_entity(test_input);
	delete test_output;
	delete test_input;
    delete cable_activated_local;
    delete cable_deactivated_local;
    clear_callback_queue();
}

void test_add_filter() {
	ZstLog::info("Starting addition filter test");
	int first_cmp_val = 4;
	int second_cmp_val = 30;

	ZstLog::debug("Creating input/output components for addition filter");
	OutputComponent * test_output_augend = new OutputComponent("add_test_augend");
	OutputComponent * test_output_addend = new OutputComponent("add_test_addend");
	InputComponent * test_input_sum = new InputComponent("add_test_sum", first_cmp_val, true);
	AddFilter * add_filter = new AddFilter("add_test");
	
	zst_activate_entity(test_output_augend);
	zst_activate_entity(test_output_addend);
	zst_activate_entity(test_input_sum);
	zst_activate_entity(add_filter);
    
	ZstLog::debug("Connecting cables");
	zst_connect_cable(add_filter->augend(), test_output_augend->output() );
	zst_connect_cable(add_filter->addend(), test_output_addend->output());
	zst_connect_cable(test_input_sum->input(), add_filter->sum());
    
	//Send values
	ZstLog::debug("Sending values");
	test_output_augend->send(2);
	test_output_addend->send(2);

	TAKE_A_BREATH

	int max_wait = 100000;
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
	ZstLog::debug("Addition component succeeded at addition!");

	//Cleanup
	zst_deactivate_entity(test_output_augend);
	zst_deactivate_entity(test_output_addend);
	zst_deactivate_entity(test_input_sum);
	zst_deactivate_entity(add_filter);
	clear_callback_queue();
	delete test_output_augend;
	delete test_output_addend;
	delete test_input_sum;
	delete add_filter;
}


void test_external_entities(std::string external_test_path) {
	ZstLog::info("Starting external entities test");

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
	OutputComponent * output_ent = new OutputComponent("proxy_test_output");
	zst_activate_entity(output_ent);
    
	//Run sink in external process so we don't share the same Showtime singleton
	ZstLog::debug("Starting sink process");
	
	ZstURI sink_perf_uri = ZstURI("sink");
	ZstURI sink_ent_uri = sink_perf_uri + ZstURI("sink_ent");
	ZstURI sink_B_uri = sink_ent_uri + ZstURI("sinkB");
	ZstURI sink_plug_uri = sink_ent_uri + ZstURI("in");

	//Run the sink program
	std::string prog = external_test_path + "/TestSink";
#ifdef WIN32
	prog += ".exe";
#endif
	boost::process::child sink_process;
#ifdef PAUSE_SINK
	char pause_flag = 'd';
#else
	char pause_flag = 'a';
#endif
	try {
		sink_process = boost::process::child(prog, &pause_flag); //d flag pauses the sink process to give us time to attach a debugger
#ifdef PAUSE_SINK
#ifdef WIN32
		system("pause");
#endif
        system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif
	}
	catch (boost::process::process_error e) {
		ZstLog::error("Sink process failed to start. Code:{} Message:{}", e.code().value(), e.what());
	}
	assert(sink_process.valid());

	//Test performer arriving
	wait_for_event(performerArriveCallback, 1);
	ZstPerformer * sink_performer = zst_get_performer_by_URI(sink_perf_uri);
	assert(sink_performer);
    
	//Test entity exists
    wait_for_event(entityArriveCallback, 1);
	ZstContainer * sink_ent = dynamic_cast<ZstContainer*>(zst_find_entity(sink_ent_uri));
	assert(sink_ent);
	entityArriveCallback->reset_num_calls();
    
	ZstPlug * sink_plug = sink_ent->get_plug_by_URI(sink_plug_uri);
	assert(sink_plug);
	assert(sink_plug->is_activated());

	//Connect cable to sink
	ZstCable * cable = zst_connect_cable(sink_plug, output_ent->output());
    assert(cable);
    assert(cable->is_activated());
    
    // TODO: We need to wait for the other end to finish connecting
    // Eventually, we need to solve this by broadcasting an empty graph
    // message on the client owning the output socket that will be picked
    // up by the subscriber when the connection is valid. At this point,
    // the subscribing client can let the stage know that the cable is
    // valid
    TAKE_A_BREATH
    
    //Send message to sink
	output_ent->send(1);
    
	//Test entity arriving
	wait_for_event(entityArriveCallback, 1);
	assert(zst_find_entity(sink_B_uri));
	entityArriveCallback->reset_num_calls();

	//Send another value to remove the child
	//Test entity leaving
	entityLeaveCallback->reset_num_calls();
	output_ent->send(2);
	wait_for_event(entityLeaveCallback, 1);
	assert(!zst_find_entity(sink_B_uri));
	entityArriveCallback->reset_num_calls();

	output_ent->send(0);
	sink_process.wait();
    int result = sink_process.exit_code();
    assert(result == 0);

	//Check that we received performer destruction request
	wait_for_event(performerLeaveCallback, 1);
	assert(!zst_get_performer_by_URI(sink_perf_uri));
    
	//Clean up output
	zst_deactivate_entity(output_ent);

	//Cleanup
    zst_remove_component_event_listener(entityArriveCallback, ZstEventAction::ARRIVING);
    zst_remove_component_event_listener(entityLeaveCallback, ZstEventAction::LEAVING);
	zst_remove_performer_event_listener(performerArriveCallback, ZstEventAction::ARRIVING);
	zst_remove_performer_event_listener(performerLeaveCallback, ZstEventAction::LEAVING);
	delete entityArriveCallback;
	delete entityLeaveCallback;
	delete performerArriveCallback;
	delete performerLeaveCallback;
	delete output_ent;
	clear_callback_queue();
}


void test_memory_leaks() {
	ZstLog::info("Starting memory leak test");

	ZstLog::debug("Creating entities and cables");
	OutputComponent * test_output = new OutputComponent("memleak_test_out");
	InputComponent * test_input = new InputComponent("memleak_test_in", 10, false);
	zst_activate_entity(test_output);
	zst_activate_entity(test_input);
	zst_connect_cable(test_input->input(), test_output->output());

	int count = MEM_LEAK_LOOPS;
    zst_reset_graph_recv_tripmeter();
    zst_reset_graph_send_tripmeter();

	ZstLog::debug("Sending {} messages", count);
	// Wait until our message tripmeter has received all the messages
	auto delta = std::chrono::milliseconds(-1);
	std::chrono::time_point<std::chrono::system_clock> end, last, now;
	auto start = std::chrono::system_clock::now();
	last = start;
	int last_message_count = 0;
	int message_count = 0;
	int delta_messages = 0;
	double mps = 0.0;
	int remaining_messages = count;
	int queued_messages = 0;
	int delta_queue = 0;
	int last_queue_count = 0;
	long queue_speed = 0;

	for (int i = 0; i < count; ++i) {
		test_output->send(10);
		zst_poll_once();
		if (zst_graph_recv_tripmeter() % 10000 == 0) {
			//Display progress
			message_count = test_input->num_hits;
			queued_messages = zst_graph_send_tripmeter() - zst_graph_recv_tripmeter();

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

			ZstLog::debug("Processing {} messages per/s. Remaining: {}, Delta T: {} per 10000, Queued: {}, Queuing speed: {} messages per/s", mps, remaining_messages, (delta.count() / 1000.0), queued_messages, queue_speed);
		}
	}
	
	ZstLog::debug("Sent all messages. Waiting for recv");

	do  {
		zst_poll_once();
		if (zst_graph_recv_tripmeter() % 10000 == 0) {
			//Display progress
			message_count = zst_graph_recv_tripmeter();
			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			queued_messages = zst_graph_send_tripmeter() - zst_graph_recv_tripmeter();
			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			remaining_messages = count - message_count;
			last_message_count = message_count;
			ZstLog::debug("Processing {} messages per/s. Remaining: {}, Delta T: {} per 10000, Queued: {}, Queuing speed: {} messages per/s", mps, remaining_messages, (delta.count() / 1000.0), queued_messages, queue_speed);
		}
	} while ((test_input->num_hits < count));

	assert(test_input->num_hits == count);
	ZstLog::debug("Received all messages. Sent: {}, Received:{}", count, test_input->num_hits);
    
	zst_deactivate_entity(test_output);
	zst_deactivate_entity(test_input);
	delete test_output;
	delete test_input;
	clear_callback_queue();
	zst_reset_graph_recv_tripmeter();
	zst_reset_graph_send_tripmeter();
}

void test_leaving(){
    //Test leave async (returns immediately)
    zst_leave_immediately();
}

void test_cleanup() {
	//Test late entity destruction after library cleanup
	zst_destroy();
}

int main(int argc,char **argv){
	std::string ext_test_folder = boost::filesystem::system_complete(argv[0]).parent_path().generic_string();
	
	//Tests
	test_startup();
	test_URI();
	test_root_entity();
    test_create_entities();
	test_hierarchy();
	test_connect_plugs();
	test_add_filter();
	test_external_entities(ext_test_folder);
	test_memory_leaks();
    test_leaving();
	test_cleanup();
	
	std::cout << "All tests completed" << std::endl;
	
#ifdef WIN32
	system("pause");
#endif

	return 0;
}
