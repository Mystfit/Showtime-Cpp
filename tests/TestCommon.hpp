#pragma once

#include <string>
#include <Showtime.h>
#include <ShowtimeServer.h>
#include <ZstFilesystemUtils.h>

#include <boost/process.hpp>
#include <boost/stacktrace.hpp>
#include <boost/thread/thread.hpp>
#include <boost/dll.hpp>
#include <memory>
#include <sstream>

// Visual studio debugging
#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#include <signal.h>

// Test includes
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>

namespace utf = boost::unit_test;
using namespace boost::process;
using namespace boost::unit_test;
using namespace showtime;

#define BOOST_THREAD_DONT_USE_DATETIME
#define TAKE_A_SHORT_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(10));
#define TAKE_A_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(100));
#define WAIT_UNTIL_STAGE_TIMEOUT std::this_thread::sleep_for(std::chrono::milliseconds(STAGE_TIMEOUT + 1000));
#define WAIT_UNTIL_STAGE_BEACON std::this_thread::sleep_for(std::chrono::milliseconds(3000));
#define TEST_TIMEOUT *timeout(10)
#define TEST_SERVER_NAME "test_server"

// --------------
// Entities
// --------------

namespace ZstTest
{
	class OutputComponent : public ZstComponent
	{
	private:
		std::unique_ptr<ZstOutputPlug> m_output;

	public:
		OutputComponent(const char * name, bool reliable = true) : 
			ZstComponent("TESTER", name),
			m_output(std::make_unique<ZstOutputPlug>("out", ZstValueType::IntList, reliable))
		{
		}

		void on_activation() override {
			Log::app(Log::Level::debug, "{} on_activation()", URI().path());
		}

		virtual void on_registered() override {
			add_child(m_output.get());
		}

		void on_deactivation() override {
			Log::app(Log::Level::debug, "{} on_deactivation()", URI().path());
		}

		virtual void compute(ZstInputPlug * plug) override {}

		void send(int val) {
			m_output->append_int(val);
			m_output->fire();
		}

		ZstOutputPlug * output() {
			return m_output.get();
		}
	};



	class InputComponent : public ZstComponent
	{
	private:
		std::unique_ptr<ZstInputPlug> m_input;

	public:
		int num_hits = 0;
		int compare_val = 0;
		int last_received_val = 0;
		bool log = false;

		InputComponent(const char * name, int cmp_val=0, bool should_log=false) :
			ZstComponent("TESTER", name),
			compare_val(cmp_val),
			log(should_log),
			m_input(std::make_unique<ZstInputPlug>("in", ZstValueType::IntList))
		{
		}

		void on_activation() override {
			Log::app(Log::Level::debug, "{} on_activation()", URI().path());
		}

		virtual void on_registered() override {
			add_child(m_input.get());
		}

		void on_deactivation() override {
			Log::app(Log::Level::debug, "{} on_deactivation()", URI().path());
		}

		virtual void compute(ZstInputPlug * plug) override {

			last_received_val = plug->int_at(0);
			if (log) {
				Log::app(Log::Level::debug, "Input filter received value {0:d}", last_received_val);
			}
			num_hits++;
		}

		ZstInputPlug * input() {
			return m_input.get();
		}

		void reset() {
			num_hits = 0;
		}
	};


	class Sink : public ZstComponent {
	public:
		std::unique_ptr<ZstInputPlug> input;
		std::unique_ptr<ZstOutputPlug> output;

		int last_received_code;
		Sink* m_child_sink;

		Sink(const char* name) :
			ZstComponent("SINK", name),
			input(std::make_unique<ZstInputPlug>("in", ZstValueType::IntList)),
			output(std::make_unique<ZstOutputPlug>("out", ZstValueType::FloatList)),
			last_received_code(0),
			m_child_sink(NULL)
		{
		}

		~Sink() {
			//Plug is automatically deleted by owning component
			input = NULL;
			m_child_sink = NULL;
		}

		virtual void on_registered() override {
			add_child(input.get());
			add_child(output.get());
		}

		virtual void compute(ZstInputPlug* plug) override {
			Log::entity(Log::Level::debug, "In sink compute");
			int request_code = plug->int_at(0);
			Log::entity(Log::Level::notification, "Sink received code {}. Echoing over output", request_code);
			output->append_int(request_code);
			output->fire();

			switch (request_code)
			{
			case 0:
				//No-op
				break;
			case 1:
				m_child_sink = new Sink("sinkB");
				this->add_child(m_child_sink);
				Log::entity(Log::Level::debug, "Sink about to sync activate child entity", m_child_sink->URI().path());
				if (!m_child_sink->is_activated())
					throw std::runtime_error("Child entity failed to activate");
				Log::entity(Log::Level::debug, "Finished sync activate");
				break;
			case 2:
				Log::entity(Log::Level::debug, "Sink about to sync deactivate child entity", m_child_sink->URI().path());
				if (!m_child_sink)
					throw std::runtime_error("Child entity is null");

				if (!m_child_sink->is_activated())
					throw std::runtime_error("Child entity is not activated");

				m_child_sink->deactivate();
				Log::entity(Log::Level::debug, "Finished sync deactivate");
				delete m_child_sink;
				m_child_sink = NULL;
				break;
			case 3:
				throw std::runtime_error("Testing compute failure.");
			case 4:
				//No-op
				break;
			default:
				break;
			}

			last_received_code = request_code;
		}
	};


	// -----------------
	// Adaptors
	// -----------------

	class TestAdaptor
	{
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


	class TestConnectionEvents : public ZstConnectionAdaptor, public TestAdaptor
	{
	public:
		bool is_connected = false;
		bool is_synced = false;
		std::vector<ZstServerAddress> discovered_servers;
		std::vector<ZstServerAddress> lost_servers;

		void on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
			Log::app(Log::Level::debug, "CONNECTION_ESTABLISHED: {}", client->get_root()->URI().path());
			inc_calls();
			is_connected = true;
		}

		void on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
			Log::app(Log::Level::debug, "DISCONNECTING: {}",client->get_root()->URI().path());
			inc_calls();
			is_connected = false;
		}
        
        void on_server_discovered(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
            Log::app(Log::Level::debug, "SERVER DISCOVERED: Name: {} Address: {}", stage_address.name, stage_address.address);
            inc_calls();
			discovered_servers.push_back(stage_address);
        }

		void on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
			Log::app(Log::Level::debug, "SERVER SYNCHRONISED");
			inc_calls();
			is_synced = true;
		}

		void on_server_lost(ShowtimeClient* client, const ZstServerAddress& stage_address) override {
			Log::app(Log::Level::debug, "SERVER LOST: Name: {} Address: {}", stage_address.name, stage_address.address);
			inc_calls();
			lost_servers.push_back(stage_address);
		}
	};


	class TestEntityEvents : public ZstHierarchyAdaptor, public TestAdaptor
	{
	public:
		ZstURI last_entity_arriving;
		ZstURI last_entity_leaving;
		ZstURI last_entity_updated;

		void on_entity_arriving(ZstEntityBase * entity) override {
			Log::app(Log::Level::debug, "ENTITY_ARRIVING: {}", entity->URI().path());
			last_entity_arriving = entity->URI();
			inc_calls();
		}

		void on_entity_leaving(const ZstURI& entity_path) override {
			Log::app(Log::Level::debug, "ENTITY_LEAVING: {}", entity_path.path());
			last_entity_leaving = entity_path;
			inc_calls();
		}

		void on_entity_updated(ZstEntityBase* entity) override {
			Log::app(Log::Level::debug, "ENTITY_UPDATED: {}", entity->URI().path());
			last_entity_updated = entity->URI();
			inc_calls();
		}
	};


	class TestSessionEvents : public ZstSessionAdaptor, public TestAdaptor
	{
	public:
		ZstCableAddress last_cable_arrived;
		ZstCableAddress last_cable_left;

		void on_cable_created(ZstCable* cable) override {
			Log::app(Log::Level::debug, "CABLE_ARRIVING: {}", cable->get_address().to_string());
			last_cable_arrived = cable->get_address();
			inc_calls();
		}

		void on_cable_destroyed(const ZstCableAddress& cable_address) override {
			Log::app(Log::Level::debug, "CABLE_LEAVING: {}", cable_address.to_string());
			last_cable_left = cable_address;
			inc_calls();
		}
	};


	class TestPerformerEvents : public ZstHierarchyAdaptor, public TestAdaptor
	{
	public:
        ZstURI last_arrived_performer;
        ZstURI last_left_performer;
        
		void on_performer_arriving(ZstPerformer * performer) override {
			Log::app(Log::Level::debug, "PERFORMER_ARRIVING: {}", performer->URI().path());
            last_arrived_performer = performer->URI();
			inc_calls();
		}

		void on_performer_leaving(const ZstURI& performer_path) override {
			Log::app(Log::Level::debug, "PERFORMER_LEAVING: {}", performer_path.path());
            last_left_performer = performer_path;
			inc_calls();
		}
	};


	class TestSynchronisableEvents : public ZstSynchronisableAdaptor, public TestAdaptor
	{
		void on_synchronisable_activated(ZstSynchronisable * synchronisable) override {
			Log::app(Log::Level::debug, "SYNCHRONISABLE_ACTIVATED: {}", synchronisable->instance_id());
			inc_calls();
		}

		void on_synchronisable_deactivated(ZstSynchronisable * synchronisable) override {
			Log::app(Log::Level::debug, "SYNCHRONISABLE_DEACTIVATED: {}", synchronisable->instance_id());
			inc_calls();
		}

		void on_synchronisable_updated(ZstSynchronisable * synchronisable) override {
			Log::app(Log::Level::debug, "SYNCHRONISABLE_UPDATED: {}", synchronisable->instance_id());
			inc_calls();
		}
	};


	class TestPlugSync : public ZstSynchronisableAdaptor, public TestAdaptor
	{
		int last_val_received = 0;
		void on_synchronisable_updated(ZstSynchronisable * synchronisable) override {
			ZstPlug * plug = dynamic_cast<ZstPlug*>(synchronisable);
			Log::app(Log::Level::debug, "SYNCHRONISABLE_UPDATED: Plug {} updated: {}", plug->URI().path(), plug->int_at(0));
			inc_calls();
		}
	};



	// ------------------
	// Events and polling
	// ------------------

#define MAX_WAIT_LOOPS 50
    void wait_for_event(std::shared_ptr<ShowtimeClient> client, std::shared_ptr<TestAdaptor> adaptor, int expected_messages)
	{
		int repeats = 0;
		client->poll_once();
		while (adaptor->num_calls() < expected_messages) {
			TAKE_A_BREATH
			repeats++;
			if (repeats > MAX_WAIT_LOOPS) {
				std::ostringstream err;
				err << "Not enough events in queue. Expecting " << expected_messages << " received " << adaptor->num_calls() << std::endl;
				throw std::runtime_error(err.str());
			}
			client->poll_once();
		}
	}

	void log_external(boost::process::ipstream & stream) {
		std::string line;

		while (std::getline(stream, line)){
			try {
				boost::this_thread::interruption_point();
				Log::app(Log::Level::debug, "\n -> {}", line.c_str());
			}
			catch (boost::thread_interrupted) {
				Log::server(Log::Level::debug, "External process logger thread exiting");
				break;
			}
		}
	}

	boost::thread log_external_pipe(boost::process::ipstream & out_pipe) {
		return boost::thread(boost::bind(&ZstTest::log_external, boost::ref(out_pipe)));
	}

	class EventLoop {
	public:
		EventLoop(std::shared_ptr<ShowtimeClient> client) : m_client(client) {}

		void operator()() {
			while (1) {
				try {
					boost::this_thread::interruption_point();
					m_client->poll_once();
					//std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				catch (boost::thread_interrupted) {
					Log::net(Log::Level::debug, "Benchmark event loop exiting.");
					break;
				}
			}
		}
	private:
		std::shared_ptr<ShowtimeClient> m_client;
	};


	class FixtureInit {
	public:
        std::shared_ptr<ShowtimeClient> test_client;
        
        FixtureInit() : test_client(std::make_shared<ShowtimeClient>())
		{
#ifdef WIN32
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
			test_client->init("test_performer", true);
            //test_client->poll_once();
		}

		~FixtureInit()
		{
			test_client->destroy();
			Log::app(Log::Level::notification, "-------------------------------------------------------------------");
		}
	};

	class FixtureInitAndCreateServerWithEpheremalPort : public FixtureInit {
	public:
		std::string server_name;
		std::string server_address;
		std::shared_ptr<ShowtimeServer> test_server;
		int server_port;

		FixtureInitAndCreateServerWithEpheremalPort()
		{
			server_name = boost::unit_test::framework::current_test_case().full_name();
			test_server = std::make_unique<ShowtimeServer>(server_name);
			server_port = test_server->port();
			server_address = fmt::format("127.0.0.1:{}", server_port);

			TAKE_A_BREATH
			test_client->poll_once();
		}

		~FixtureInitAndCreateServerWithEpheremalPort()
		{
			test_server->destroy();
		}
	};


	class FixtureJoinServer : public FixtureInitAndCreateServerWithEpheremalPort {
	public:
		FixtureJoinServer()
		{
			test_client->auto_join_by_name(server_name.c_str());
			test_client->poll_once();
		}

		~FixtureJoinServer()
		{
		}
	};


	class FixtureEventLoop {
	public:
		FixtureEventLoop(std::shared_ptr<ShowtimeClient> client) : 
			m_eventloop_thread(boost::thread(EventLoop(client))) 
		{
		}

		~FixtureEventLoop() {
			m_eventloop_thread.interrupt();
			m_eventloop_thread.join();
		}
	private:
		boost::thread m_eventloop_thread;
	};


	class FixtureJoinEventLoop : 
		public FixtureJoinServer, 
		public FixtureEventLoop 
	{
	public:
		FixtureJoinEventLoop() : FixtureEventLoop(test_client) {}
	};


	class FixtureExternalClient {
	public:
		boost::process::child external_process;
		boost::process::ipstream external_process_stdout;
		boost::process::pipe external_process_stdin;

		ZstURI external_performer_URI;

		FixtureExternalClient(std::string program_name)
		{
			external_performer_URI = ZstURI(program_name.c_str());

            Log::app(Log::Level::debug, "Current path is{}", fs::current_path().generic_string());
            auto program_path = fs::path(boost::unit_test::framework::master_test_suite().argv[0]).parent_path().append(program_name);
#ifdef WIN32
			program_path.replace_extension("exe");
#endif

			std::string test_flag = "test";

			//Run client as an external process so we don't share the same Showtime singleton
			Log::app(Log::Level::notification, "Starting {} process", program_path.generic_string());
			try {
				external_process = boost::process::child(program_path.generic_string(), test_flag.c_str(), boost::process::std_in < external_process_stdin, boost::process::std_out > external_process_stdout); //d flag pauses the sink process to give us time to attach a debugger
#ifdef PAUSE_SINK
#ifdef WIN32
				system("pause");
#endif
				system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif
				TAKE_A_BREATH
			}
			catch (boost::process::process_error e) {
				Log::app(Log::Level::error, "External process failed to start. Code:{} Message:{}", e.code().value(), e.what());
			}

			// Create a thread to handle reading log info from the sink process' stdout pipe
			external_process_log_thread = ZstTest::log_external_pipe(external_process_stdout);
		}

		~FixtureExternalClient() {
			external_process.terminate();
			external_process_stdout.pipe().close();

			external_process_log_thread.interrupt();
			external_process_log_thread.join();
		}

	private:
		boost::thread external_process_log_thread;
	};


	struct FixtureRemoteClient {
		ZstURI external_performer_URI;
		std::shared_ptr<ShowtimeClient> remote_client;

		FixtureRemoteClient(std::string client_name, std::string server_name) :
			remote_client(std::make_shared<ShowtimeClient>()),
			external_performer_URI(client_name.c_str())
		{
			remote_client->init(client_name.c_str(), true);
			remote_client->auto_join_by_name(server_name.c_str());
		}

		~FixtureRemoteClient() {
			remote_client->destroy();
		}
	};

	struct FixtureSinkClient : public FixtureRemoteClient {
		//Common URIs
		ZstURI sink_ent_uri;
		ZstURI sink_plug_uri;
		ZstURI sync_out_plug_uri;
		std::unique_ptr<Sink> sink;

		FixtureSinkClient(std::string server_name) :
			FixtureRemoteClient("TestHelperSink", server_name),
			sink(std::make_unique<Sink>("sink_ent")),
			sink_ent_uri(remote_client->get_root()->URI() + ZstURI("sink_ent")),
			sink_plug_uri(sink_ent_uri + ZstURI("in")),
			sync_out_plug_uri(sink_ent_uri + ZstURI("out"))
		{
			remote_client->get_root()->add_child(sink.get());
		}

		~FixtureSinkClient() {
		}
	};

	struct FixtureWaitForSinkClient : public FixtureJoinServer, FixtureSinkClient {
		std::shared_ptr<TestPerformerEvents> performerEvents;

		FixtureWaitForSinkClient() :
			FixtureJoinServer(),
			FixtureSinkClient(server_name),
			performerEvents(std::make_shared<TestPerformerEvents>())
		{
			test_client->add_hierarchy_adaptor(performerEvents);
			BOOST_TEST_CHECKPOINT("Waiting for external client performer to arrive");
			wait_for_event(test_client, performerEvents, 1);
			performerEvents->reset_num_calls();
		}
		~FixtureWaitForSinkClient() {}
	};
};

int s_interrupted = 0;
typedef void (*SignalHandlerPointer)(int);

void s_signal_handler(int signal_value){
	switch (signal_value) {
	case SIGINT:
		s_interrupted = 1;
	case SIGTERM:
		s_interrupted = 1;
	case SIGABRT:
		s_interrupted = 1;
	case SIGSEGV:
		s_interrupted = 1;
		signal(signal_value, SIG_DFL);
		boost::stacktrace::safe_dump_to("./backtrace.dump");
		raise(SIGABRT);
	default:
		break;
	}
}

void s_catch_signals(){
#ifdef WIN32
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGSEGV, &s_signal_handler);
#else
	struct sigaction action;
	action.sa_handler = s_signal_handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
#endif
}

bool init_unit_test()
{
	return true;
}

void read_stacktrace() {
	auto dumpfile = "./backtrace.dump";
//	auto dumpfile = fmt::format("./{}.dump", boost::dll::program_location().filename().string());
	if (fs::exists(dumpfile)) {
		// there is a backtrace
		std::ifstream ifs(dumpfile);

		boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
		std::cout << "Previous run crashed:\n" << st << std::endl;

		// cleaning up
		ifs.close();
		fs::remove(dumpfile);
	}
}

int main(int argc, char* argv[])
{
	read_stacktrace();
	s_catch_signals();
	return utf::unit_test_main(&init_unit_test, argc, argv);
}