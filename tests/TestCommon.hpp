#pragma once
#include <string>
#include <Showtime.h>
#include <ShowtimeServer.h>
#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/test/unit_test.hpp>
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

using namespace boost::process;

#define BOOST_THREAD_DONT_USE_DATETIME
#define TAKE_A_SHORT_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(10));
#define TAKE_A_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(100));
#define WAIT_UNTIL_STAGE_TIMEOUT std::this_thread::sleep_for(std::chrono::milliseconds(STAGE_TIMEOUT + 1000));
#define WAIT_UNTIL_STAGE_BEACON std::this_thread::sleep_for(std::chrono::milliseconds(3000));

#define TEST_SERVER_NAME "test_server"

// --------------
// Entities
// --------------

namespace ZstTest
{
	class OutputComponent : public ZstComponent
	{
	private:
		ZstOutputPlug * m_output;

	public:
		OutputComponent(const char * name, bool reliable = true) : 
			ZstComponent("TESTER", name),
			m_output(create_output_plug("out", ZstValueType::ZST_INT, reliable))
		{
		}

		void on_activation() override {
			ZstLog::app(LogLevel::debug, "{} on_activation()", URI().path());
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
			return m_output;
		}
	};



	class InputComponent : public ZstComponent
	{
	private:
		ZstInputPlug * m_input;

	public:
		int num_hits = 0;
		int compare_val = 0;
		int last_received_val = 0;
		bool log = false;

		InputComponent(const char * name, int cmp_val=0, bool should_log=false) :
			ZstComponent("TESTER", name),
			compare_val(cmp_val),
			log(should_log),
			m_input(create_input_plug("in", ZstValueType::ZST_INT))
		{
		}

		void on_activation() override {
			ZstLog::app(LogLevel::debug, "{} on_activation()", URI().path());
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
			return m_input;
		}

		void reset() {
			num_hits = 0;
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


	class TestConnectionEvents : public ZstSessionAdaptor, public TestAdaptor
	{
	public:
		bool is_connected = false;
		ZstServerAddress last_discovered_server;

		void on_connected_to_stage() override {
			ZstLog::app(LogLevel::debug, "CONNECTION_ESTABLISHED: {}", zst_get_root()->URI().path());
			inc_calls();
			is_connected = true;
		}

		void on_disconnected_from_stage() override {
			ZstLog::app(LogLevel::debug, "DISCONNECTING: {}", zst_get_root()->URI().path());
			inc_calls();

			is_connected = false;
		}
        
        void on_server_discovered(const ZstServerAddress & server) override {
            ZstLog::app(LogLevel::debug, "SERVER DISCOVERED: Name: {} Address: {}", server.name, server.address);
            inc_calls();

			last_discovered_server = server;
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
		void on_synchronisable_updated(ZstSynchronisable * synchronisable) override {
			ZstPlug * plug = dynamic_cast<ZstPlug*>(synchronisable);
			ZstLog::app(LogLevel::debug, "SYNCHRONISABLE_UPDATED: Plug {} updated: {}", plug->URI().path(), plug->int_at(0));
			inc_calls();
		}
	};



	// ------------------
	// Events and polling
	// ------------------

	inline void clear_callback_queue()
	{
		zst_poll_once();
	}

#define MAX_WAIT_LOOPS 50
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
		FixtureInit(){
			zst_init("test_performer", true);
			clear_callback_queue();
		}

		~FixtureInit()
		{
			zst_destroy();
		}
	};
	

	class FixtureInitAndCreateServer : public FixtureInit {
	public:
		FixtureInitAndCreateServer() {
			m_stage_server = zst_create_server(TEST_SERVER_NAME, STAGE_ROUTER_PORT);
			TAKE_A_BREATH
			clear_callback_queue();
		}

		~FixtureInitAndCreateServer() {
			zst_destroy_server(m_stage_server);
		}
	private:
		ServerHandle m_stage_server;
	};


	class FixtureJoinServer : public FixtureInitAndCreateServer {
	public:

		FixtureJoinServer()
		{
			zst_join(("127.0.0.1:" + std::to_string(STAGE_ROUTER_PORT)).c_str());
			clear_callback_queue();
		}

		~FixtureJoinServer()
		{
		}
	};


	class FixtureExternalClient : public FixtureJoinServer {
	public:
		boost::process::child external_process;
		ZstURI external_performer_URI;

		FixtureExternalClient(std::string program_name)
		{
			external_performer_URI = ZstURI(program_name.c_str());

			auto program_path = fs::current_path().parent_path().append("bin").append(program_name);
#ifdef WIN32
			program_path.replace_extension("exe");
#endif

#ifdef PAUSE_SINK
			char pause_flag = 'd';
#else
			char pause_flag = 'a';
#endif
			//Run client as an external process so we don't share the same Showtime singleton
			ZstLog::app(LogLevel::notification, "Starting {} process", program_path.generic_string());
			try {
				external_process = boost::process::child(program_path.generic_string(), &pause_flag, boost::process::std_out > external_process_out); //d flag pauses the sink process to give us time to attach a debugger
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
			external_process_log_thread = ZstTest::log_external_pipe(external_process_out);
		}

		~FixtureExternalClient() {
			external_process.terminate();
			external_process_out.pipe().close();

			external_process_log_thread.interrupt();
			external_process_log_thread.join();
		}

	private:
		boost::process::ipstream external_process_out;
		boost::thread external_process_log_thread;
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
