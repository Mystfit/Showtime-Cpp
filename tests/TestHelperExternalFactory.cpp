#include "TestCommon.hpp"

using namespace ZstTest;

#define CUSTOM_EXT_COMPONENT "CustomExternalComponent"

class CustomExternalComponent : public ZstComponent
{
public:
	CustomExternalComponent(const char * name) : ZstComponent(CUSTOM_EXT_COMPONENT, name) {}
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
		CustomExternalComponent * entity = NULL;
		if (creatable_path == this->URI() + ZstURI(CUSTOM_EXT_COMPONENT)) {
			entity = new CustomExternalComponent(name);
		}
		return entity;
	}
};


int main(int argc, char **argv) {

	ZstLog::app(LogLevel::notification, "In external factory process");

	bool force_launch = true;
	if (argc < 2 && !force_launch) {
		ZstLog::app(LogLevel::warn, "Skipping sink test, command line flag not set");
		return 0;
	}

	if (argc >= 2) {
		if (argv[1][0] == 'd')
#ifdef WIN32
			system("pause");
#else
			system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif
	}
	zst_init("extfactory", true);

	//Create the factory that will instantiate our custom entity
	std::shared_ptr<TestExternalFactory> test_factory = std::shared_ptr<TestExternalFactory>(new TestExternalFactory("external_customs"));
	zst_register_factory(test_factory.get());
	
	//Connect to the stage
	zst_join("127.0.0.1");
	assert(test_factory->is_activated());

	ZstTest::s_catch_signals();
	while (!ZstTest::s_interrupted) {
		zst_poll_once();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	zst_destroy();
	return 0;
}
