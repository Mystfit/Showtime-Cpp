#include "TestCommon.hpp"
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <iostream>

using namespace ZstTest;

#define CUSTOM_EXT_COMPONENT "CustomExternalComponent"


std::shared_ptr<ShowtimeClient> test_client;


class CustomExternalComponent : public ZstComponent
{
public:
	std::unique_ptr<ZstInputPlug> input;
	CustomExternalComponent(const char* name) :
		ZstComponent(CUSTOM_EXT_COMPONENT, name),
		input(std::make_unique <ZstInputPlug>("input", ZstValueType::IntList)){}

	virtual void on_registered() override {
		add_child(input.get());
	}

protected:
	void compute(ZstInputPlug * plug) override {
		ZstLog::entity(LogLevel::notification, "Custom external component {} received a value", this->URI().path());
	}
};


class TestExternalFactory : public ZstEntityFactory
{
public:
	TestExternalFactory(const char * name) : ZstEntityFactory(name)
	{
		this->add_creatable(CUSTOM_EXT_COMPONENT);
	}

	virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name) override
	{
		ZstLog::entity(LogLevel::notification, "External factory received create request for {}", creatable_path.path());
		CustomExternalComponent * entity = NULL;
		if (creatable_path == this->URI() + ZstURI(CUSTOM_EXT_COMPONENT)) {

			//TODO: Factory created entity leakages
			ZstLog::entity(LogLevel::error, "New factory created entity will leak - replace with smart pointer!");
			entity = new CustomExternalComponent(name);
		}
		return entity;
	}
};


void event_loop()
{
	ZstLog::net(LogLevel::debug, "Starting event loop");
	while (1) {
		try {
			boost::this_thread::interruption_point();
			test_client->poll_once();
			boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
		}
		catch (boost::thread_interrupted) {
			ZstLog::net(LogLevel::debug, "Stopping event loop");
			break;
		}
	}
}


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


int main(int argc, char **argv) {

	ZstLog::app(LogLevel::notification, "In external factory process");

	bool force_launch = false;
	if (!force_launch) {
		bool skip = true;
		for (int i = 0; i < argc; ++i) {
			if (strcmp(argv[i], "test") == 0) {
				skip = false;
			}
		}
		if (skip) {
			ZstLog::app(LogLevel::warn, "Skipping, 'test' command line flag not set");
			return 0;
		}
	}
	
	test_client = std::make_shared<ShowtimeClient>();
	test_client->init("extfactory", true);

	//Create the factory that will instantiate our custom entity
	auto test_factory = std::make_shared<TestExternalFactory>("external_customs");
	test_client->register_factory(test_factory.get());
	
	//Connect to the stage
	test_client->auto_join_by_name(TEST_SERVER_NAME);
	assert(test_factory->is_activated());

	//Start event loop
	boost::thread event_thread = boost::thread(event_loop);

	s_catch_signals();
	while (!ZstTest::s_interrupted) {
		std::string line;

		//Update creatables when we get a message on stdin
		std::getline(std::cin, line);
		if (line == "update_creatables") {
			ZstLog::app(LogLevel::debug, "Received std::cin request to update creatables");
			test_factory->add_creatable(ZstURI("avocado"));
			test_factory->update_creatables();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	//Close event loop
	event_thread.interrupt();
	event_thread.join();

	test_client->destroy();
	return 0;
}
