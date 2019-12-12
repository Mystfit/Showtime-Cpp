#pragma once
#include <string>
#include <Showtime.h>
#include <ShowtimeServer.h>
#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <sstream>

#ifdef __cpp_lib_filesystem
#include <filesystem>
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#endif

// Visual studio debugging
#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

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
			m_output(std::make_unique<ZstOutputPlug>("out", ValueList_IntList, reliable))
		{
		}

		void on_activation() override {
			ZstLog::app(LogLevel::debug, "{} on_activation()", URI().path());
		}

		virtual void on_registered() override {
			add_child(m_output.get());
		}

		void on_deactivation() override {
			ZstLog::app(LogLevel::debug, "{} on_deactivation()", URI().path());
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
			m_input(std::make_unique<ZstInputPlug>("in", ValueList_IntList))
		{
		}

		void on_activation() override {
			ZstLog::app(LogLevel::debug, "{} on_activation()", URI().path());
		}

		virtual void on_registered() override {
			add_child(m_input.get());
		}

		void on_deactivation() override {
			ZstLog::app(LogLevel::debug, "{} on_deactivation()", URI().path());
		}

		virtual void compute(ZstInputPlug * plug) override {

			last_received_val = plug->int_at(0);
			if (log) {
				ZstLog::app(LogLevel::debug, "Input filter received value {0:d}", last_received_val);
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
			input(std::make_unique<ZstInputPlug>("in", ValueList_IntList)),
			output(std::make_unique<ZstOutputPlug>("out", ValueList_FloatList)),
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
			ZstLog::entity(LogLevel::debug, "In sink compute");
			int request_code = plug->int_at(0);
			ZstLog::entity(LogLevel::notification, "Sink received code {}. Echoing over output", request_code);
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
				ZstLog::entity(LogLevel::debug, "Sink about to sync activate child entity", m_child_sink->URI().path());
				if (!m_child_sink->is_activated())
					throw std::runtime_error("Child entity failed to activate");
				ZstLog::entity(LogLevel::debug, "Finished sync activate");
				break;
			case 2:
				ZstLog::entity(LogLevel::debug, "Sink about to sync deactivate child entity", m_child_sink->URI().path());
				if (!m_child_sink)
					throw std::runtime_error("Child entity is null");

				if (!m_child_sink->is_activated())
					throw std::runtime_error("Child entity is not activated");

				m_child_sink->deactivate();
				ZstLog::entity(LogLevel::debug, "Finished sync deactivate");
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
		ZstServerAddress last_discovered_server;

		void on_connected_to_stage(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
			ZstLog::app(LogLevel::debug, "CONNECTION_ESTABLISHED: {}", client->get_root()->URI().path());
			inc_calls();
			is_connected = true;
		}

		void on_disconnected_from_stage(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
			ZstLog::app(LogLevel::debug, "DISCONNECTING: {}",client->get_root()->URI().path());
			inc_calls();
			is_connected = false;
		}
        
        void on_server_discovered(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
            ZstLog::app(LogLevel::debug, "SERVER DISCOVERED: Name: {} Address: {}", stage_address.name, stage_address.address);
            inc_calls();
			last_discovered_server = stage_address;
        }

		void on_synchronised_with_stage(ShowtimeClient* client, const ZstServerAddress & stage_address) override {
			ZstLog::app(LogLevel::debug, "SERVER SYNCHRONISED");
			inc_calls();
			is_synced = true;
		}

	private:
	};


	class TestEntityEvents : public ZstHierarchyAdaptor, public TestAdaptor
	{
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


	class TestPerformerEvents : public ZstHierarchyAdaptor, public TestAdaptor
	{
	public:
        ZstURI last_arrived_performer;
        ZstURI last_left_performer;

        
		void on_performer_arriving(ZstPerformer * performer) override {
			ZstLog::app(LogLevel::debug, "PERFORMER_ARRIVING: {}", performer->URI().path());
            last_arrived_performer = performer->URI();
			inc_calls();
		}

		void on_performer_leaving(ZstPerformer * performer) override {
			ZstLog::app(LogLevel::debug, "PERFORMER_LEAVING: {}", performer->URI().path());
            last_left_performer = performer->URI();
			inc_calls();
		}
	};


	class TestSynchronisableEvents : public ZstSynchronisableAdaptor, public TestAdaptor
	{
		void on_synchronisable_activated(ZstSynchronisable * synchronisable) override {
			ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_ACTIVATED: {}", synchronisable->instance_id());
			inc_calls();
		}

		void on_synchronisable_deactivated(ZstSynchronisable * synchronisable) override {
			ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_DEACTIVATED: {}", synchronisable->instance_id());
			inc_calls();
		}

		void on_synchronisable_updated(ZstSynchronisable * synchronisable) override {
			ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_UPDATED: {}", synchronisable->instance_id());
			inc_calls();
		}
	};


	class TestPlugSync : public ZstSynchronisableAdaptor, public TestAdaptor
	{
		int last_val_received = 0;
		void on_synchronisable_updated(ZstSynchronisable * synchronisable) override {
			ZstPlug * plug = dynamic_cast<ZstPlug*>(synchronisable);
			ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_UPDATED: Plug {} updated: {}", plug->URI().path(), plug->int_at(0));
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
				ZstLog::app(LogLevel::debug, "\n -> {}", line.c_str());
			}
			catch (boost::thread_interrupted) {
				ZstLog::server(LogLevel::debug, "External process logger thread exiting");
				break;
			}
		}
	}

	boost::thread log_external_pipe(boost::process::ipstream & out_pipe) {
		return boost::thread(boost::bind(&ZstTest::log_external, boost::ref(out_pipe)));
	}

	class FixtureInit {
	public:
        std::shared_ptr<ShowtimeClient> test_client;
        
        FixtureInit() : test_client(std::make_shared<ShowtimeClient>())
		{
#ifdef WIN32
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
			test_client->init("test_performer", true);
            test_client->poll_once();
		}

		~FixtureInit()
		{
			test_client->destroy();
			ZstLog::app(LogLevel::notification, "-------------------------------------------------------------------");
		}
	};
	

	class FixtureInitAndCreateServer : public FixtureInit {
	public:
		FixtureInitAndCreateServer() 
		{
			m_stage_server = std::make_shared< ShowtimeServer>(TEST_SERVER_NAME, STAGE_ROUTER_PORT);
			TAKE_A_BREATH
			test_client->poll_once();
		}

		~FixtureInitAndCreateServer() 
		{
			m_stage_server->destroy();
		}
	private:
		std::shared_ptr<ShowtimeServer> m_stage_server;
	};


	class FixtureJoinServer : public FixtureInitAndCreateServer {
	public:

		FixtureJoinServer()
		{
			test_client->join(("127.0.0.1:" + std::to_string(STAGE_ROUTER_PORT)).c_str());
			test_client->poll_once();
		}

		~FixtureJoinServer()
		{
		}
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

            ZstLog::app(LogLevel::debug, "Current path is{}", fs::current_path().generic_string());
            auto program_path = fs::path(boost::unit_test::framework::master_test_suite().argv[0]).parent_path().append(program_name);
#ifdef WIN32
			program_path.replace_extension("exe");
#endif

			std::string test_flag = "test";

			//Run client as an external process so we don't share the same Showtime singleton
			ZstLog::app(LogLevel::notification, "Starting {} process", program_path.generic_string());
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
				ZstLog::app(LogLevel::error, "External process failed to start. Code:{} Message:{}", e.code().value(), e.what());
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


	struct FixtureLocalClient {
		ZstURI external_performer_URI;
		std::shared_ptr<ShowtimeClient> local_client;

		FixtureLocalClient(std::string client_name) :
			local_client(std::make_shared<ShowtimeClient>()),
			external_performer_URI(client_name.c_str())
		{
			local_client->init(client_name.c_str(), true);
			local_client->auto_join_by_name(TEST_SERVER_NAME);
		}

		~FixtureLocalClient() {
			local_client->destroy();
		}
	};

	// Signal catching
	// -----------

#ifdef WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

	static int s_interrupted = 0;
};
