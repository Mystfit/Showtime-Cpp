#pragma once
#include <string>
#include <Showtime.h>
#include <ShowtimeServer.h>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <sstream>

using namespace boost::process;
using namespace boost::filesystem;

#define BOOST_THREAD_DONT_USE_DATETIME
#define TAKE_A_SHORT_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(10));
#define TAKE_A_BREATH std::this_thread::sleep_for(std::chrono::milliseconds(100));
#define WAIT_UNTIL_STAGE_TIMEOUT std::this_thread::sleep_for(std::chrono::milliseconds(STAGE_TIMEOUT + 1000));


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
		OutputComponent(const char * name, bool reliable = true) : ZstComponent("TESTER", name) {
			m_output = create_output_plug("out", ZstValueType::ZST_INT, reliable);
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

		InputComponent(const char * name, int cmp_val, bool should_log=false) :
			ZstComponent("TESTER", name), compare_val(cmp_val)
		{
			log = should_log;
			m_input = create_input_plug("in", ZstValueType::ZST_INT);
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
		void on_connected_to_stage() override {
			ZstLog::app(LogLevel::debug, "CONNECTION_ESTABLISHED: {}", zst_get_root()->URI().path());
			inc_calls();
		}

		void on_disconnected_from_stage() override {
			ZstLog::app(LogLevel::debug, "DISCONNECTING: {}", zst_get_root()->URI().path());
			inc_calls();
		}
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
		void on_performer_arriving(ZstPerformer * performer) override {
			ZstLog::app(LogLevel::debug, "PERFORMER_ARRIVING: {}", performer->URI().path());
			inc_calls();
		}

		void on_performer_leaving(ZstPerformer * performer) override {
			ZstLog::app(LogLevel::debug, "PERFORMER_LEAVING: {}", performer->URI().path());
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


	class TestRunner {
	public:
		TestRunner(const std::string & name, const std::string & test_path, bool init_library = true, bool run_stage = true)
		{
			if (run_stage) {
				m_stage_server = zst_create_server((name + "_server").c_str(), STAGE_ROUTER_PORT);
			}

			//Init library
			if (init_library) {
				zst_init(name.c_str(), true);
				zst_join("127.0.0.1");
				if (!zst_is_connected()) {
					ZstLog::app(LogLevel::error, "Failed to connect to launched stage");
					assert(zst_is_connected());
				}
			}
		}

		~TestRunner()
		{
			zst_destroy_server(m_stage_server);
			zst_destroy();
		}

	private:
		ServerHandle m_stage_server;
	};


	// Signal catching
	// -----------

#ifdef WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

	static int s_interrupted = 0;

#ifdef WIN32
	static bool s_signal_handler(DWORD signal_value)
#else
	static void s_signal_handler(int signal_value)
#endif
	{
		ZstLog::app(LogLevel::debug, "Caught signal {}", signal_value);
		switch (signal_value) {
#ifdef WIN32
		case CTRL_C_EVENT:
			s_interrupted = 1;
			return true;
		case CTRL_CLOSE_EVENT:
			s_interrupted = 1;
			return true;
		default:
			break;
		}
		return false;
#else
		case SIGINT:
			s_interrupted = 1;
		case SIGTERM:
			s_interrupted = 1;
		case SIGKILL:
			s_interrupted = 1;
		case SIGABRT:
			s_interrupted = 1;
		default:
			break;
	}
#endif
	}

	static void s_catch_signals() {
#ifdef WIN32
		if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)s_signal_handler, TRUE)) {
			ZstLog::app(LogLevel::error, "Unable to register Control Handler");
		}
#else
		struct sigaction action;
		action.sa_handler = s_signal_handler;
		action.sa_flags = 0;
		sigemptyset(&action.sa_mask);
		sigaction(SIGINT, &action, NULL);
		sigaction(SIGTERM, &action, NULL);
#endif
	}
};
