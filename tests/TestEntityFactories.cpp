#include "TestCommon.hpp"
#include <string>
#include <memory>

using namespace ZstTest;

#define CUSTOM_COMPONENT "CustomComponent"

class CustomComponent : public ZstComponent 
{
public:
    CustomComponent(const char * name) : ZstComponent(CUSTOM_COMPONENT, name) {}
protected:
    void compute(ZstInputPlug * plug) override {
        ZstLog::entity(LogLevel::notification, "Custom component {} received a value", this->URI().path());
    }
};

class TestFactory : public ZstEntityFactory
{
public:
	TestFactory(const char * name) : ZstEntityFactory(name) 
	{
		this->add_creatable(CUSTOM_COMPONENT);
	}

	virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name) override
	{
		CustomComponent * entity = NULL;
		if (creatable_path == this->URI() + ZstURI(CUSTOM_COMPONENT)) {
			entity = new CustomComponent(name);
		}
		return entity;
	}
};

void test_entity_factories(){
	//Create the factory that will instantiate our custom entity
	std::shared_ptr<TestFactory> test_factory = std::shared_ptr<TestFactory>(new TestFactory("customs"));
	zst_register_factory(test_factory.get());
	assert(test_factory->is_activated());

	ZstURIBundle bundle;
	test_factory->get_creatables(bundle);
	assert(bundle.size() == 1);
	assert(bundle[0] == test_factory->URI() + ZstURI(CUSTOM_COMPONENT));

	//Create an scoped entity that will be owned by the application (not the Showtime library)
	{
		ZstSharedEntity entity = ZstSharedEntity(zst_create_entity(bundle[0], "brand_spanking_new"));
		assert(entity);
		assert(zst_find_entity(zst_get_root()->URI() + ZstURI("brand_spanking_new")));
	}

	//Test scoped entity destruction
	assert(!zst_find_entity(bundle[0]));
}

void test_remote_factories(std::string external_test_path, bool launch_ext_process = true)
{
	//Adaptors
	TestPerformerEvents * performerEvents = new TestPerformerEvents();
	zst_add_hierarchy_adaptor(performerEvents);
	
	//Run the sink program
	std::string prog = system_complete(external_test_path).parent_path().generic_string() + "/TestHelperExternalFactory";
#ifdef WIN32
	prog += ".exe";
#endif
	boost::process::child ext_factory_process;
	if (launch_ext_process) {
		//Run external factory in external process so we don't share the same Showtime singleton
		ZstLog::app(LogLevel::debug, "Starting external factory process");
		try {
			ext_factory_process = boost::process::child(prog);
		}
		catch (boost::process::process_error e) {
			ZstLog::app(LogLevel::debug, "External factory process failed to start. Code:{} Message:{}", e.code().value(), e.what());
		}
		assert(ext_factory_process.valid());
		wait_for_event(performerEvents, 1);
	}

	ZstPerformer * ext_factory_performer = dynamic_cast<ZstPerformer*>(zst_find_entity(ZstURI("extfactory")));
	assert(ext_factory_performer);
	performerEvents->reset_num_calls();

	ZstEntityFactory * ext_factory = dynamic_cast<ZstEntityFactory*>(zst_find_entity(ZstURI("extfactory/external_customs")));
	assert(ext_factory);

	ZstURIBundle bundle;
	ext_factory->get_creatables(bundle);
	assert(bundle.size() == 1);
	assert(bundle[0] == ext_factory->URI() + ZstURI("CustomExternalComponent"));

	//Create an external entity using the external factory
	ZstEntityBase * entity = zst_create_entity(bundle[0], "brand_spanking_new_ext");
	assert(entity);
	assert(zst_find_entity(ext_factory_performer->URI() + ZstURI("brand_spanking_new_ext")));

	if(launch_ext_process)
		ext_factory_process.terminate();
}


int main(int argc,char **argv)
{
    TestRunner runner("TestEntityFactories", argv[0]);
    test_entity_factories();
	test_remote_factories(argv[0]);
    return 0;
}
