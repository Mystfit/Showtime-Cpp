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

#ifdef WIN32
#define WAIT_FOR_HEARTBEAT Sleep(HEARTBEAT_DURATION);
#else
#define WAIT_FOR_HEARTBEAT usleep(1000 * HEARTBEAT_DURATION);
#endif

#define MAX_WAIT 20
void wait_for_event(ZstEvent * callback, int expected_messages)
{
	int repeats = 0;
	while (callback->num_calls() < expected_messages) {
		TAKE_A_BREATH
		repeats++;
		if (repeats > MAX_WAIT) {
			std::ostringstream err;
			err << "Not enough events in queue. Expecting " << expected_messages << " received " << callback->num_calls() << std::endl;
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

class TestConnectCallback : public ZstClientConnectionEvent {
public:
	std::string m_suffix;
	TestConnectCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstPerformer * root_performer) override {
		LOGGER->debug("CONN_EVENT: {} {}", root_performer->URI().path(), m_suffix);
	}
};

class TestEntityActivatedCallback : public ZstEntityEvent {
public:
	std::string m_suffix;
	TestEntityActivatedCallback(std::string suffix) {
		m_suffix = suffix;
	}
	void run(ZstEntityBase * entity) override {
		LOGGER->debug("ENTITY_ACTIVATED_EVENT: {} {}", entity->URI().path(), m_suffix);
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
		LOGGER->debug("CABLE_EVENT: {} {} {}", cable->get_output()->URI().path(), cable->get_input()->URI().path(), m_suffix);
	}
};

class TestEntityTemplateCallback : public ZstComponentTypeEvent {
public:
	std::string m_suffix;
	TestEntityTemplateCallback(std::string suffix) {
		m_suffix = suffix;
	}
    void run(ZstEntityBase * entity_template) override {
		LOGGER->debug("TEMPLATE_EVENT: {} Owner {}", entity_template->entity_type(), entity_template->parent()->URI().path(), m_suffix);
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

	InputComponent(const char * name, int cmp_val) : 
		ZstComponent("TESTER", name), compare_val(cmp_val) {
	}

	virtual void init() override {
		m_input = create_input_plug("in", ZstValueType::ZST_INT);
	}

	virtual void compute(ZstInputPlug * plug) override {
		num_hits++;
		last_received_val = plug->int_at(0);
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
}

void test_startup() {

	LOGGER->info("Running Showtime init test");

	//Test connection
	ZstClientConnectionEvent * connectCallback = new TestConnectCallback("connected");
	Showtime::attach_callback(connectCallback);

	Showtime::init("TestClient");
	Showtime::join("127.0.0.1");

	wait_for_event(connectCallback, 1);
	assert(connectCallback->num_calls() == 1);
	
	Showtime::detach_callback(connectCallback);
	delete connectCallback;

	//assert(Showtime::ping() >= 0);
}

void test_root_entity() {
	LOGGER->info("Running entity init test");

	//Test root entity is activated
    ZstPerformer * root_entity = Showtime::get_root();
	assert(root_entity);
    assert(root_entity->is_activated());
	clear_callback_queue();
}


void test_create_entities(){
	LOGGER->info("Running entity test");
	int expected_entities = 1;
	int expected_plugs = expected_entities * 1;

	TestEntityActivatedCallback * entity_activated = new TestEntityActivatedCallback("activated");
	Showtime::attach_callback(entity_activated);

	//Create entities
	OutputComponent * test_output = new OutputComponent("entity_create_test_ent");
	Showtime::activate(test_output);

	wait_for_event(entity_activated, 1);
	assert(test_output->is_activated());
    
	//Check local client registered plugs correctly
	ZstURI localPlug_uri = test_output->get_plug_by_URI(test_output->output()->URI())->URI();
	ZstURI localPlug_uri_via_entity = test_output->output()->URI();
	assert(ZstURI::equal(localPlug_uri, localPlug_uri_via_entity));

	//Cleanup
	Showtime::deactivate(test_output);
	Showtime::detach_callback(entity_activated);

	delete test_output;
	delete entity_activated;
	test_output = 0;
	entity_activated = 0;

	assert(!Showtime::get_root()->find_child_by_URI(localPlug_uri));
}


void test_hierarchy() {
	LOGGER->info("Running hierarchy test");

	//Test hierarchy
	ZstContainer * parent = new ZstContainer("parent");
	ZstComponent * child = new ZstComponent("child");
	parent->add_child(child);
	Showtime::activate(parent);
	
	assert(Showtime::get_root()->find_child_by_URI(parent->URI()));
	assert(Showtime::get_root()->find_child_by_URI(child->URI()));

	LOGGER->debug("Removing child...");

	//Test child removal from parent
	parent->remove_child(child);
	ZstURI parent_URI = ZstURI(parent->URI());
	ZstURI child_URI = ZstURI(child->URI());
	delete child;
	child = 0;
	assert(!parent->find_child_by_URI(child_URI));
	assert(!Showtime::get_root()->find_child_by_URI(child_URI));

	//Test removing parent removes child
	LOGGER->debug("Creating new child to test parent removes all children");

	child = new ZstComponent("child");
	parent->add_child(child);
	Showtime::activate(child);
	
	delete parent;
	assert(!Showtime::get_root()->find_child_by_URI(parent_URI));
	assert(!Showtime::get_root()->find_child_by_URI(child_URI));
	delete child;
	child = 0;
	parent = 0;
	
	clear_callback_queue();
}


void test_connect_plugs() {
	LOGGER->info("Running connect plugs test");
	
	int expected_entities = 2;
	int expected_plugs = expected_entities;
	int expected_cables = 1;
	OutputComponent * test_output = new OutputComponent("connect_test_ent_out");
	InputComponent * test_input = new InputComponent("connect_test_ent_in", 0);
	
	//Test cable callbacks
	TestCableEventCallback * cableArriveCallback = new TestCableEventCallback("arriving");
	TestCableEventCallback * cableLeaveCallback = new TestCableEventCallback("leaving");
    Showtime::attach_callback(cableArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::attach_callback(cableLeaveCallback, ZstCallbackAction::LEAVING);

	//Test connect cable
	ZstCable * cable = Showtime::connect_cable(test_output->output(), test_input->input());
	wait_for_event(cableArriveCallback, expected_cables);
	assert(cableArriveCallback->num_calls() == expected_cables);
	cableArriveCallback->reset_num_calls();

	//Test cable references
	assert(test_output->output()->num_cables() == 1);
	assert(cable->get_output() == test_output->output());
	assert(cable->get_input() == test_input->input());
	assert(test_output->output()->is_connected_to(test_input->input()));
	for (auto c : *(test_output->output())) {
		assert(c->get_input() == test_input->input());
	}

	//Testing connection disconnect and reconnect
	Showtime::destroy_cable(cable);
	wait_for_event(cableLeaveCallback, 1);
	assert(cableLeaveCallback->num_calls() == 1);
	cableLeaveCallback->reset_num_calls();
	assert(!test_output->output()->is_connected_to(test_input->input()));

	//Test plug deactivation causes cable destruction
	Showtime::connect_cable(test_output->output(), test_input->input());
	Showtime::deactivate(test_output);
	wait_for_event(cableLeaveCallback, 1);
	assert(cableLeaveCallback->num_calls() == 1);
	assert(!test_input->input()->is_connected_to(test_output->output()));
	cableLeaveCallback->reset_num_calls();

	delete test_output;
	delete test_input;
	test_output = 0;
	test_input = 0;

	//Test removing callbacks
    Showtime::detach_callback(cableArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::detach_callback(cableLeaveCallback, ZstCallbackAction::LEAVING);
	delete cableArriveCallback;
	delete cableLeaveCallback;
	clear_callback_queue();
}


void test_add_filter() {
	LOGGER->info("Starting addition filter test");
		
	int expected_entities = 4;
	int expected_plugs = 6;
	int expected_cables = 3;
	int first_cmp_val = 4;
	int second_cmp_val = 30;

	//Create a test filter to hold out in/out plugs
	OutputComponent * test_output_augend = new OutputComponent("add_test_augend");
	OutputComponent * test_output_addend = new OutputComponent("add_test_addend");
	InputComponent * test_input_sum = new InputComponent("add_test_sum", first_cmp_val);
	Showtime::activate(test_output_augend);
	Showtime::activate(test_output_addend);
	Showtime::activate(test_input_sum);

	test_input_sum->log = true;
	AddFilter * add_filter = new AddFilter("add_test");
	Showtime::activate(add_filter);

	Showtime::connect_cable(test_output_augend->output(), add_filter->augend());
	Showtime::connect_cable(test_output_addend->output(), add_filter->addend());
	Showtime::connect_cable(test_input_sum->input(), add_filter->sum());
	clear_callback_queue();

	//Send values
	test_output_augend->send(2);
	test_output_addend->send(2);

	//Wait for the first two input callbacks to clear before we check for the sum
    while(test_input_sum->num_hits < 2){
		Showtime::poll_once();
    }
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

	Showtime::deactivate(test_output_augend);
	Showtime::deactivate(test_output_addend);
	Showtime::deactivate(test_input_sum);
	Showtime::deactivate(add_filter);
	delete test_output_augend;
	delete test_output_addend;
	delete test_input_sum;
	delete add_filter;
	test_output_augend = 0;
	test_output_addend = 0;
	test_input_sum = 0;
	add_filter = 0;
	clear_callback_queue();
}


void test_external_entities(std::string external_sink_path) {
	LOGGER->info("Starting external entities test");

	//Create callbacks
	TestEntityEventCallback * entityArriveCallback = new TestEntityEventCallback("arriving");
	TestEntityEventCallback * entityLeaveCallback = new TestEntityEventCallback("leaving");
	TestPerformerCallback * performerArriveCallback = new TestPerformerCallback("arriving");
	TestPerformerCallback * performerLeaveCallback = new TestPerformerCallback("leaving");

    Showtime::attach_callback(entityArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::attach_callback(entityLeaveCallback, ZstCallbackAction::LEAVING);
	Showtime::attach_callback(performerArriveCallback, ZstCallbackAction::ARRIVING);
	Showtime::attach_callback(performerLeaveCallback, ZstCallbackAction::LEAVING);

	//Create emitter
	OutputComponent * output = new OutputComponent("proxy_test_output");
	Showtime::activate(output);
	TAKE_A_BREATH
	clear_callback_queue();

	//Run sink in external process so we don't share the same Showtime singleton
	LOGGER->debug("Starting sink process");
	
	ZstURI sink_perf_uri = ZstURI("sink");
	ZstURI sink_ent_uri = sink_perf_uri + ZstURI("sink_ent");
	ZstURI sink_B_uri = sink_perf_uri + ZstURI("sinkB");
	ZstURI sink_plug_uri = sink_ent_uri + ZstURI("in");

	//Run the sink program
	std::string prog = external_sink_path + "/SinkTest";
#ifdef WIN32
	prog += ".exe";
#endif
	boost::process::child sink_process = boost::process::child(prog, "1");

	//Test performer arriving
	wait_for_event(performerArriveCallback, 1);
	ZstPerformer * sink_performer = Showtime::get_performer_by_URI(sink_perf_uri);
	assert(sink_performer);
	
	//Test entity exists
	ZstContainer * sink_ent = dynamic_cast<ZstContainer*>(sink_performer->find_child_by_URI(sink_ent_uri));
	assert(sink_ent);

	//Connect cable to sink
	Showtime::connect_cable(output->output(), sink_ent->get_plug_by_URI(sink_plug_uri));
	TAKE_A_BREATH
	output->send(1);

	//Test entity arriving
	wait_for_event(entityArriveCallback, 1);
	assert(entityArriveCallback->last_entity == std::string(sink_B_uri.path()));
	assert(Showtime::find_entity(sink_B_uri));
	entityArriveCallback->reset_num_calls();

	//Send another value to remove the child
	//Test entity leaving
	output->send(2);
	wait_for_event(entityLeaveCallback, 1);
	assert(entityLeaveCallback->last_entity == std::string(sink_B_uri.path()));
	assert(!Showtime::find_entity(sink_B_uri));
	entityArriveCallback->reset_num_calls();

	output->send(0);
	sink_process.wait();

	//Check that we received performer destruction request
	wait_for_event(performerLeaveCallback, 1);
	assert(!Showtime::get_performer_by_URI(sink_perf_uri));

	//Cleanup
    Showtime::detach_callback(entityArriveCallback, ZstCallbackAction::ARRIVING);
    Showtime::detach_callback(entityLeaveCallback, ZstCallbackAction::LEAVING);
	Showtime::detach_callback(performerArriveCallback, ZstCallbackAction::ARRIVING);
	Showtime::detach_callback(performerLeaveCallback, ZstCallbackAction::LEAVING);
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
	Showtime::activate(test_output);
	Showtime::activate(test_input);
	Showtime::connect_cable(test_output->output(), test_input->input());
	TAKE_A_BREATH

	int count = num_loops;

	//ZstClient::instance().reset_graph_recv_tripmeter();
	//ZstClient::instance().reset_graph_send_tripmeter();

	LOGGER->debug("Sending {0:d} messages", count);

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
	//	Showtime::poll_once();
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
	Showtime::poll_once();
	LOGGER->debug("Received all messages");
	/*std::cout << "Remaining events: " << Showtime::event_queue_size() << std::endl;
	std::cout << "Total received graph_messages " << ZstClient::instance().graph_recv_tripmeter() << std::endl;*/
	assert(test_input->num_hits == count);

	delete test_output;
	delete test_input;
	clear_callback_queue();
	/*ZstClient::instance().reset_graph_recv_tripmeter();
	ZstClient::instance().reset_graph_send_tripmeter();*/
}

void test_leaving(){
    Showtime::leave();
}


void test_cleanup() {
	//Test late entity destruction after library cleanup
	Showtime::destroy();
}

int main(int argc,char **argv){
	ZST_init_log();
	LOGGER->set_level(spdlog::level::debug);
	test_standard_layout();
	test_URI();
	test_startup();
	test_root_entity();
    test_create_entities();
	test_hierarchy();
	test_connect_plugs();
	test_add_filter();
	test_external_entities(boost::filesystem::system_complete(argv[0]).parent_path().generic_string());
	test_memory_leaks(200000);
    test_leaving();
	test_cleanup();
	LOGGER->info("All tests completed");

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
