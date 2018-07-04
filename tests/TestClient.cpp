#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <sstream>
#include <exception>

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>  
#include <crtdbg.h>  
#pragma warning(push) 
#pragma warning(disable:4189 4996)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic pop
#endif
#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
using namespace boost::process;

#ifdef WIN32
#pragma warning(pop)
#endif

#include <Showtime.h>
#include "../src/core_entities/Adder.h"

#define BOOST_THREAD_DONT_USE_DATETIME
#define MEM_LEAK_LOOPS 200000;

#define TAKE_A_SHORT_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(10));
#define TAKE_A_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(100));
#define WAIT_UNTIL_STAGE_TIMEOUT std::this_thread::sleep_for(std::chrono::milliseconds(STAGE_TIMEOUT + 1000));


inline void clear_callback_queue() {
	zst_poll_once();
}

class TestAdaptor {
public:
	TestAdaptor() : m_num_calls(0) {};

	int num_calls() {
		return m_num_calls;
	}

	void reset_num_calls() {
		m_num_calls = 0;
	}

	void inc_calls() {
		m_num_calls++;
	}

private:
	int m_num_calls;
};



//Callback classes
//----------------
class TestConnectionEvents : public ZstSessionAdaptor, public TestAdaptor {
public:
	void on_connected_to_stage() override {
		ZstLog::app(LogLevel::debug, "CONNECTION_ESTABLISHED: {}", zst_get_root()->URI().path());
		inc_calls();
	}

	void on_disconnected_from_stage() override {
		ZstLog::app(LogLevel::debug, "DISCONNECTING: {}", zst_get_root()->URI().path());
		inc_calls();
	}
};

class TestEntityEvents : public ZstHierarchyAdaptor, public TestAdaptor {
public:
	std::string last_entity_arriving;
	std::string last_entity_leaving;

	void on_entity_arriving(ZstEntityBase * entity) override {
		ZstLog::app(LogLevel::debug, "ENTITY_ARRIVING: {}", entity->URI().path());
		last_entity_arriving = std::string(entity->URI().path());
		inc_calls();
	}

	void on_entity_leaving(ZstEntityBase * entity) override {
		ZstLog::app(LogLevel::debug, "ENTITY_LEAVING: {}", entity->URI().path());
		last_entity_leaving = std::string(entity->URI().path());
		inc_calls();
	}
};

class TestPlugEvents : public ZstHierarchyAdaptor, public TestAdaptor {
public:
	std::string last_plug_arriving;
	std::string last_plug_leaving;

	void on_plug_arriving(ZstPlug * plug) override {
		ZstLog::app(LogLevel::debug, "PLUG_ARRIVING: {}", plug->URI().path());
		last_plug_leaving = std::string(plug->URI().path());
		inc_calls();
	}

	void on_plug_leaving(ZstPlug * plug) override {
		ZstLog::app(LogLevel::debug, "PLUG_LEAVING: {}", plug->URI().path());
		last_plug_leaving = std::string(plug->URI().path());
		inc_calls();
	}
};

class TestCableEvents : public ZstSessionAdaptor, public TestAdaptor {
public:
	void on_cable_created(ZstCable * cable) override {
		ZstLog::app(LogLevel::debug, "CABLE_ARRIVING: {} {}", cable->get_output_URI().path(), cable->get_input_URI().path());
		inc_calls();
	}

	void on_cable_destroyed(ZstCable * cable) override {
		ZstLog::app(LogLevel::debug, "CABLE_LEAVING: {} {}", cable->get_output_URI().path(), cable->get_input_URI().path());
		inc_calls();
	}
};

class TestPerformerEvents : public ZstHierarchyAdaptor, public TestAdaptor {
	void on_performer_arriving(ZstPerformer * performer) override {
		ZstLog::app(LogLevel::debug, "PERFORMER_ARRIVING: {}", performer->URI().path());
		inc_calls();
	}

	void on_performer_leaving(ZstPerformer * performer) override {
		ZstLog::app(LogLevel::debug, "PERFORMER_LEAVING: {}", performer->URI().path());
		inc_calls();
	}
};

class TestSynchronisableEvents : public ZstSynchronisableAdaptor, public TestAdaptor {
	void on_synchronisable_activated(ZstSynchronisable * synchronisable) override {
		ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_ACTIVATED");
		inc_calls();
	}

	void on_synchronisable_deactivated(ZstSynchronisable * synchronisable) override {
		ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_DEACTIVATED");
		inc_calls();
	}
};


#define MAX_WAIT_LOOPS 200
void wait_for_event(TestAdaptor * adaptor, int expected_messages)
{
	int repeats = 0;
	zst_poll_once();
	while (adaptor->num_calls() < expected_messages) {
		TAKE_A_BREATH
			repeats++;
		if (repeats > MAX_WAIT_LOOPS) {
			std::ostringstream err;
			err << "Not enough events in queue. Expecting " << expected_messages << " received " << adaptor->num_calls() << std::endl;
			throw std::runtime_error(err.str());
		}
		zst_poll_once();
	}
}


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

	ZstOutputPlug * output() {
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
		float actual_val = plug->float_at(0);
		last_received_val = int(actual_val);
		if (log) {
			ZstLog::app(LogLevel::debug, "Input filter received value {0:d}", last_received_val);
		}
		num_hits++;
	}

	ZstInputPlug * input() {
		return m_input;
	}

	void reset() {
		num_hits = 0;
	}
};


void test_startup() {
	zst_init("TestClient", true);
	zst_start_file_logging();
	ZstLog::app(LogLevel::notification, "Running Showtime init test");
	
	//Test sync join
	ZstLog::app(LogLevel::debug, "Testing sync join");
	zst_join("127.0.0.1");
	assert(zst_is_connected());
	zst_leave();
	assert(!zst_is_connected());

	//Test sync join again to verify we cleaned up properly the first time
	ZstLog::app(LogLevel::debug, "Testing sync join again");
	zst_join("127.0.0.1");
	assert(zst_is_connected());

	//Testing join not starting if we're already connected
	ZstLog::app(LogLevel::debug, "Testing abort connection start if we're already connected");
	zst_join("127.0.0.1");
	assert(!zst_is_connecting());
	zst_leave();
	assert(!zst_is_connected());

	//Test async join
	TestConnectionEvents * connectCallback = new TestConnectionEvents();
	zst_add_session_adaptor(connectCallback);

	ZstLog::app(LogLevel::debug, "Testing async join");
	assert(connectCallback->num_calls() == 0);
	zst_join_async("127.0.0.1");
	wait_for_event(connectCallback, 1);
	assert(connectCallback->num_calls() == 1);
	assert(zst_is_connected());
	connectCallback->reset_num_calls();
	zst_leave();
	assert(!zst_is_connected());

	//Test join timeout
	ZstLog::app(LogLevel::debug, "Testing sync join timeout");
	zst_join("255.255.255.255");
	assert(!zst_is_connected());

	//Test async join timeout
	ZstLog::app(LogLevel::debug, "Testing async join timeout");
	zst_join_async("255.255.255.255");
	WAIT_UNTIL_STAGE_TIMEOUT
	assert(!zst_is_connected());
	
	//Testing abort connection start if we're already connecting
	ZstLog::app(LogLevel::debug, "Testing abort connection start if we're already connecting");
	zst_join_async("255.255.255.255");
	assert(zst_is_connecting());
	zst_join("255.255.255.255");
	assert(!zst_is_connected());
	WAIT_UNTIL_STAGE_TIMEOUT
	assert(!zst_is_connecting());

	//Stay connected at the end
	zst_join("127.0.0.1");

	//Cleanup
	zst_remove_session_adaptor(connectCallback);
	delete connectCallback;
}

void test_URI() {

	//Run URI self test
	ZstURI::self_test();

	//RUn cable self test
	ZstCable::self_test();
}

void test_root_entity() {
	ZstLog::app(LogLevel::notification, "Running performer test");
	
	//Test root entity is activated
	TestSynchronisableEvents * performer_activated = new TestSynchronisableEvents();
    ZstPerformer * root_entity = zst_get_root();
	assert(root_entity);
    
    //This should execute immediately since we've already connected to the stage
	root_entity->add_adaptor(performer_activated);
	assert(performer_activated->num_calls() == 1);
	performer_activated->reset_num_calls();
    assert(root_entity->is_activated());
	clear_callback_queue();
	root_entity->remove_adaptor(performer_activated);
	delete performer_activated;
	ZstLog::app(LogLevel::debug, "Root performer is activated");
}


void test_create_entities(){
	ZstLog::app(LogLevel::notification, "Running entity creation test");
    
    OutputComponent * test_output_sync = new OutputComponent("entity_create_test_sync");
    
    ZstLog::app(LogLevel::debug, "Testing entity sync activation");
    zst_activate_entity(test_output_sync);
    assert(test_output_sync->is_activated());
    assert(zst_find_entity(test_output_sync->URI()));
    
    ZstLog::app(LogLevel::debug, "Testing entity sync deactivation");
    zst_deactivate_entity(test_output_sync);
    assert(!test_output_sync->is_activated());
    assert(!zst_find_entity(test_output_sync->URI()));
    delete test_output_sync;
    
    //Test async entity
	OutputComponent * test_output_async = new OutputComponent("entity_create_test_async");
	TestSynchronisableEvents * entity_sync = new TestSynchronisableEvents();
	test_output_async->add_adaptor(entity_sync);
		
    ZstLog::app(LogLevel::debug, "Testing entity async activation");
	zst_activate_entity_async(test_output_async);
	wait_for_event(entity_sync, 1);
	assert(entity_sync->num_calls() == 1);
	entity_sync ->reset_num_calls();
    
	//Check local client registered plugs correctly
	assert(test_output_async->is_activated());
	ZstURI localPlug_uri = test_output_async->get_plug_by_URI(test_output_async->output()->URI())->URI();
	ZstURI localPlug_uri_via_entity = test_output_async->output()->URI();
	assert(ZstURI::equal(localPlug_uri, localPlug_uri_via_entity));

    ZstLog::app(LogLevel::debug, "Testing entity async deactivation");
	zst_deactivate_entity(test_output_async);
	wait_for_event(entity_sync, 1);
	assert(entity_sync->num_calls() == 1);
	entity_sync->reset_num_calls();
	assert(!test_output_async->is_activated());
	assert(!zst_find_entity(test_output_async->URI()));
    assert(!zst_find_entity(localPlug_uri));
    
    //Cleanup
	delete test_output_async;
	delete entity_sync;
}


void test_hierarchy() {
	ZstLog::app(LogLevel::notification, "Running hierarchy test");

	//Test hierarchy
	ZstContainer * parent = new ZstContainer("parent");
	ZstComponent * child = new ZstComponent("child");
	parent->add_child(child);

	zst_activate_entity(parent);
	assert(zst_find_entity(parent->URI()));
	assert(zst_find_entity(child->URI()));
	
	//Test child removal from parent
	ZstLog::app(LogLevel::debug, "Testing child removal from parent");
	ZstURI child_URI = ZstURI(child->URI());
	zst_deactivate_entity(child);
	assert(!parent->find_child_by_URI(child_URI));
	assert(!zst_find_entity(child_URI));
    
    //Test child activation and deactivation callbacks
	ZstLog::app(LogLevel::debug, "Test child activation and deactivation callbacks");
    TestSynchronisableEvents * child_activation = new TestSynchronisableEvents();
    child->add_adaptor(child_activation);
    parent->add_child(child);
    
    zst_activate_entity(child);
    assert(child_activation->num_calls() == 1);
	child_activation->reset_num_calls();
    zst_deactivate_entity(child);
    assert(child_activation->num_calls() == 1);
	child_activation->reset_num_calls();
    child->remove_adaptor(child_activation);
    delete child_activation;
    
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
	ZstLog::app(LogLevel::notification, "Running connect plugs test");
	
	ZstLog::app(LogLevel::debug, "Creating entities");
	OutputComponent * test_output = new OutputComponent("connect_test_out");
	InputComponent * test_input = new InputComponent("connect_test_in", 0);
    zst_activate_entity(test_output);
	zst_activate_entity(test_input);

	ZstLog::app(LogLevel::debug, "Testing sync cable connection");
	ZstCable * cable = zst_connect_cable(test_input->input(), test_output->output());
    assert(cable->is_activated());
	
	ZstLog::app(LogLevel::debug, "Verifying cable");
	assert(cable->get_output() == test_output->output());
	assert(cable->get_input() == test_input->input());
    assert(test_output->output()->is_connected_to(test_input->input()));
    assert(test_input->input()->is_connected_to(test_output->output()));
	for (auto c : *(test_output->output())) {
		assert(c->get_input() == test_input->input());
	}

	ZstLog::app(LogLevel::debug, "Testing cable disconnection");
	zst_destroy_cable(cable);
    cable = 0;
    assert(!test_output->output()->is_connected_to(test_input->input()));
    assert(!test_input->input()->is_connected_to(test_output->output()));
    
    ZstLog::app(LogLevel::debug, "Testing async cable connection");
    TestSynchronisableEvents * cable_activation = new TestSynchronisableEvents();
    cable = zst_connect_cable_async(test_input->input(), test_output->output());
    cable->add_adaptor(cable_activation);
    wait_for_event(cable_activation, 1);
    assert(cable_activation->num_calls() == 1);
	cable_activation->reset_num_calls();
	    
    zst_destroy_cable_async(cable);
    cable = 0;
    wait_for_event(cable_activation, 1);
    assert(cable_activation->num_calls() == 1);
	cable_activation->reset_num_calls();

	ZstLog::app(LogLevel::debug, "Testing cable disconnection when removing parent");
	cable = zst_connect_cable(test_input->input(), test_output->output());
    cable->add_adaptor(cable_activation);
    zst_deactivate_entity(test_output);
    wait_for_event(cable_activation, 1);
	assert(!test_input->input()->is_connected_to(test_output->output()));
    assert(!test_output->output()->is_connected_to(test_input->input()));
    
	//Cleanup
	zst_deactivate_entity(test_input);
	delete test_output;
	delete test_input;
    delete cable_activation;
    clear_callback_queue();
}

void test_add_filter() {
	ZstLog::app(LogLevel::notification, "Starting addition filter test");
	int first_cmp_val = 4;
	int second_cmp_val = 30;

	ZstLog::app(LogLevel::debug, "Creating input/output components for addition filter");
	OutputComponent * test_output_augend = new OutputComponent("add_test_augend");
	OutputComponent * test_output_addend = new OutputComponent("add_test_addend");
	InputComponent * test_input_sum = new InputComponent("add_test_sum", first_cmp_val, true);
	Adder * add_filter = new Adder("add_test");
	
	zst_activate_entity(test_output_augend);
	zst_activate_entity(test_output_addend);
	zst_activate_entity(test_input_sum);
	zst_activate_entity(add_filter);
    
	ZstLog::app(LogLevel::debug, "Connecting cables");
	zst_connect_cable(add_filter->augend(), test_output_augend->output() );
	zst_connect_cable(add_filter->addend(), test_output_addend->output());
	zst_connect_cable(test_input_sum->input(), add_filter->sum());
	
	TAKE_A_BREATH

	//Send values
	ZstLog::app(LogLevel::debug, "Sending values");
	test_output_augend->send(2);
	test_output_addend->send(2);

	TAKE_A_BREATH

	int max_wait = 10000;
	int current_wait = 0;

	//Wait for the first two input callbacks to clear before we check for the sum
    while(test_input_sum->num_hits < 2 && ++current_wait < max_wait){
		zst_poll_once();
    }
	assert(test_input_sum->last_received_val == first_cmp_val);
	test_input_sum->reset();
	current_wait = 0;

	//Send more values
	test_input_sum->compare_val = second_cmp_val;
	test_output_augend->send(20);
	test_output_addend->send(10);

	while (test_input_sum->num_hits < 2 && ++current_wait < max_wait){
		zst_poll_once();
	}
	assert(test_input_sum->last_received_val == second_cmp_val);
	ZstLog::app(LogLevel::debug, "Addition component succeeded at addition!");

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
	ZstLog::app(LogLevel::notification, "Starting external entities test");

	//Create callbacks
	TestEntityEvents * entityEvents = new TestEntityEvents();
	TestPerformerEvents * performerEvents = new TestPerformerEvents();
	
	zst_add_hierarchy_adaptor(entityEvents);
	zst_add_hierarchy_adaptor(performerEvents);

	//Create emitter
	OutputComponent * output_ent = new OutputComponent("proxy_test_output");
	zst_activate_entity(output_ent);
    
	//Run sink in external process so we don't share the same Showtime singleton
	ZstLog::app(LogLevel::debug, "Starting sink process");
	
	ZstURI sink_perf_uri = ZstURI("sink");
	ZstURI sink_ent_uri = sink_perf_uri + ZstURI("sink_ent");
	ZstURI sink_B_uri = sink_ent_uri + ZstURI("sinkB");
	ZstURI sink_plug_uri = sink_ent_uri + ZstURI("in");

	//Run the sink program
	bool launched_sink_process = true;
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
	if (launched_sink_process) {
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
			ZstLog::app(LogLevel::debug, "Sink process failed to start. Code:{} Message:{}", e.code().value(), e.what());
		}
		assert(sink_process.valid());

		//Test performer arriving
		wait_for_event(performerEvents, 1);
	}
	ZstPerformer * sink_performer = zst_get_performer_by_URI(sink_perf_uri);
	assert(sink_performer);
	performerEvents->reset_num_calls();
    
	//Test entity exists
	if (launched_sink_process) {
		wait_for_event(entityEvents, 1);
	}
	ZstContainer * sink_ent = dynamic_cast<ZstContainer*>(zst_find_entity(sink_ent_uri));
	assert(sink_ent);
	entityEvents->reset_num_calls();
    
	ZstInputPlug * sink_plug = dynamic_cast<ZstInputPlug*>(sink_ent->get_plug_by_URI(sink_plug_uri));
	assert(sink_plug);
	assert(sink_plug->is_activated());

	//Connect cable to sink
	ZstCable * cable = zst_connect_cable(sink_plug, output_ent->output());
    assert(cable);
    assert(cable->is_activated());
	
	//Send message to sink to test entity creation
	ZstLog::app(LogLevel::debug, "Asking sink to create an entity");
	output_ent->send(1);
    
	//Test entity arriving
	wait_for_event(entityEvents, 1);
	assert(zst_find_entity(sink_B_uri));
	entityEvents->reset_num_calls();

	//Send another value to remove the child
	//Test entity leaving
	ZstLog::app(LogLevel::debug, "Asking sink to remove an entity");
	output_ent->send(2);
	wait_for_event(entityEvents, 1);
	assert(!zst_find_entity(sink_B_uri));
	entityEvents->reset_num_calls();

	//Send message to sink
	ZstLog::app(LogLevel::debug, "Asking sink to throw an error");
	output_ent->send(3);
	//Not sure how to test for the error...

	ZstLog::app(LogLevel::debug, "Asking sink to leave");
	output_ent->send(0);
	sink_process.wait();
    int result = sink_process.exit_code();
    assert(result == 0);

	//Check that we received performer destruction request
	wait_for_event(performerEvents, 1);
	assert(!zst_get_performer_by_URI(sink_perf_uri));
	performerEvents->reset_num_calls();

	//Clean up output
	zst_deactivate_entity(output_ent);

	//Cleanup
    zst_remove_hierarchy_adaptor(entityEvents);
	zst_remove_hierarchy_adaptor(performerEvents);
	delete entityEvents;
	delete performerEvents;
	delete output_ent;
	clear_callback_queue();
}


void test_memory_leaks() {
	ZstLog::app(LogLevel::notification, "Starting memory leak test");

	ZstLog::app(LogLevel::debug, "Creating entities and cables");
	OutputComponent * test_output = new OutputComponent("memleak_test_out");
	InputComponent * test_input = new InputComponent("memleak_test_in", 10, false);
	zst_activate_entity(test_output);
	zst_activate_entity(test_input);
	zst_connect_cable(test_input->input(), test_output->output());

	int count = MEM_LEAK_LOOPS;

	ZstLog::app(LogLevel::debug, "Sending {} messages", count);
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
	int num_sent = 0;

	for (int i = 0; i < count; ++i) {
		test_output->send(10);
		num_sent++;
		zst_poll_once();
		if (test_input->num_hits % 10000 == 0) {
			//Display progress
			message_count = test_input->num_hits;
			queued_messages = num_sent - test_input->num_hits;

			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			delta_queue = queued_messages - last_queue_count;

			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			queue_speed = static_cast<long>(delta_queue / (delta.count() / 1000.0));

			remaining_messages = count - message_count;
			last_message_count = message_count;
			last_queue_count = queued_messages;

			ZstLog::app(LogLevel::debug, "Processing {} messages per/s. Remaining: {}, Delta T: {} per 10000, Queued: {}, Queuing speed: {} messages per/s", mps, remaining_messages, (delta.count() / 1000.0), queued_messages, queue_speed);
		}
	}
	
	ZstLog::app(LogLevel::debug, "Sent all messages. Waiting for recv");
	
	int max_loops_idle = 50;
	int num_loops_idle = 0;

	do  {
		zst_poll_once();
		if (test_input->num_hits % 10000 == 0) {
			//Display progress
			message_count = test_input->num_hits;
			now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
			delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last);
			delta_messages = message_count - last_message_count;
			queued_messages = num_sent - test_input->num_hits;
			last = now;
			mps = (long)delta_messages / (delta.count() / 1000.0);
			remaining_messages = count - message_count;
			
			//Break out of the loop if we lost some messages
			if(last_message_count == message_count){
				num_loops_idle++;
				if(num_loops_idle > max_loops_idle)
					break;
			} else {
				num_loops_idle = 0;
			}
			
			last_message_count = message_count;
			ZstLog::app(LogLevel::debug, "Processing {} messages per/s. Remaining: {}, Delta T: {} per 10000, Queued: {}, Queuing speed: {} messages per/s", mps, remaining_messages, (delta.count() / 1000.0), queued_messages, queue_speed);
		}
	} while ((test_input->num_hits < count));

	assert(test_input->num_hits == count);
	ZstLog::app(LogLevel::debug, "Received all messages. Sent: {}, Received:{}", count, test_input->num_hits);
   
	zst_deactivate_entity(test_output);
	zst_deactivate_entity(test_input);
	delete test_output;
	delete test_input;
	clear_callback_queue();
	// zst_reset_graph_recv_tripmeter();
	// zst_reset_graph_send_tripmeter();
}

void test_leaving(){
    //Test leave async (returns immediately)
    ZstLog::app(LogLevel::notification, "Starting stage leave immediate test");
    zst_leave();
    ZstLog::app(LogLevel::notification, "Left stage");
}

void test_cleanup() {
	//Test late entity destruction after library cleanup
	ZstLog::app(LogLevel::notification, "Starting library destruction test");
	zst_destroy();
	ZstLog::app(LogLevel::notification, "Library successfully destroyed");
}

int main(int argc,char **argv){

	bool testing = true;
	if (argc > 1) {
		if (argv[1][0] == 't') {
			ZstLog::app(LogLevel::warn, "In test mode. Launching internal stage server.");
			testing = true;
		}
	}
	
	std::string ext_test_folder = boost::filesystem::system_complete(argv[0]).parent_path().generic_string();
	boost::process::pipe server_in;
	child server_process;
	
	if (testing) {
		//Start server
		std::string prog = ext_test_folder + "/ShowtimeServer";
#ifdef WIN32
		prog += ".exe";
#endif
		
		char test_flag = 't';
		try {
			server_process = boost::process::child(prog, &test_flag, std_in < server_in);
		}
		catch (process_error e) {
			ZstLog::app(LogLevel::debug, "Server process failed to start. Code:{} Message:{}", e.code().value(), e.what());
		}
		assert(server_process.valid());
	}

	
	//Tests
	test_URI();
	test_startup();
	test_root_entity();
    test_create_entities();
	test_hierarchy();
	test_connect_plugs();
	test_add_filter();
	test_external_entities(ext_test_folder);
	//test_memory_leaks();
    test_leaving();
	test_cleanup();

	if (testing) {
		std::string term_msg = "$TERM\n";
		server_in.write(term_msg.c_str(), term_msg.size());
		server_process.wait();
		std::cout << "All tests completed" << std::endl;
	}

#ifdef WIN32
	//Dump memory leaks to console
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}
