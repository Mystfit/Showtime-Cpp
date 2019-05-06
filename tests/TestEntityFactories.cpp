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

class TestFactoryAdaptor : public ZstFactoryAdaptor, public TestAdaptor
{
public:
	ZstURI last_created_entity;

	void on_creatables_updated(ZstEntityFactory * factory) override
	{
		ZstLog::app(LogLevel::debug, "FACTORY_UPDATED: Creatables list modified for factory {}", factory->URI().path());
		inc_calls();
	}

	void on_entity_created(ZstEntityBase * entity) override
	{
		ZstLog::app(LogLevel::debug, "FACTORY_ENTITY_CREATED: {}", entity->URI().path());
		last_created_entity = entity->URI();
		inc_calls();
	}
};

void test_entity_factories(){
	//Create the factory that will instantiate our custom entity
	ZstLog::app(LogLevel::notification, "Test creating a local factory");
	std::shared_ptr<TestFactory> test_factory = std::shared_ptr<TestFactory>(new TestFactory("customs"));
	zst_register_factory(test_factory.get());
	assert(test_factory->is_activated());

	ZstURIBundle bundle;
	ZstLog::app(LogLevel::notification, "Test getting creatables from a factory");
	test_factory->get_creatables(bundle);
	assert(bundle.size() == 1);
	assert(bundle[0] == test_factory->URI() + ZstURI(CUSTOM_COMPONENT));

	//Create an scoped entity that will be owned by the application (not the Showtime library)
	{
		ZstLog::app(LogLevel::notification, "Test creating an entity from a factory");
		ZstSharedEntity entity = ZstSharedEntity(zst_create_entity(bundle[0], "brand_spanking_new"));
		assert(entity);
		assert(zst_find_entity(zst_get_root()->URI() + ZstURI("brand_spanking_new")));
	}

	//Test scoped entity destruction
	assert(!zst_find_entity(bundle[0]));

	//Test factory destruction
	test_factory = NULL;
}

void test_remote_factories(std::string external_test_path, bool launch_ext_process = true)
{
	//Adaptors
	TestPerformerEvents * performerEvents = new TestPerformerEvents();
	zst_add_hierarchy_adaptor(performerEvents);
	
	//Run the sink program
	std::string prog = std::filesystem::absolute(external_test_path).parent_path().generic_string() + "/TestHelperExternalFactory";
#ifdef WIN32
	prog += ".exe";
#endif
	boost::process::child ext_factory_process;
	boost::process::pipe ext_factory_pipe;
	boost::process::ipstream ext_factory_out;
	boost::thread ext_factory_log_thread;

	if (launch_ext_process) {
		//Run external factory in external process so we don't share the same Showtime singleton
		ZstLog::app(LogLevel::notification, "Starting external factory process");
		try {
			ext_factory_process = boost::process::child(prog, std_in < ext_factory_pipe, std_out > ext_factory_out);
		}
		catch (boost::process::process_error e) {
			ZstLog::app(LogLevel::error, "External factory process failed to start. Code:{} Message:{}", e.code().value(), e.what());
		}
		assert(ext_factory_process.valid());
		wait_for_event(performerEvents, 1);

		// Create a thread to handle reading log info from the external factory process' stdout pipe
		ext_factory_log_thread = ZstTest::log_external_pipe(ext_factory_out);
	}


	ZstLog::app(LogLevel::notification, "Test if we can search for a remote factory");
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
	ZstLog::app(LogLevel::notification, "Test if we can create a remote entity");
	ZstEntityBase * entity = zst_create_entity(bundle[0], "brand_spanking_new_ext");
	assert(entity);
	assert(zst_find_entity(ext_factory_performer->URI() + ZstURI("brand_spanking_new_ext")));

	//Test creating external entity async
	ZstLog::app(LogLevel::notification, "Test if we can create a remote entity async");
	std::shared_ptr<TestFactoryAdaptor> factory_adp = std::make_shared<TestFactoryAdaptor>();
	ext_factory->add_adaptor(factory_adp.get());
	zst_create_entity_async(bundle[0], "brand_spanking_new_ext_async");
	wait_for_event(factory_adp.get(), 1);
	ZstURI async_entity = ext_factory_performer->URI() + ZstURI("brand_spanking_new_ext_async");
	assert(factory_adp->last_created_entity == async_entity);
	assert(zst_find_entity(async_entity));
	assert(factory_adp->num_calls() == 1);
	factory_adp->reset_num_calls();

	//Test remote factories updating entities
	ZstLog::app(LogLevel::notification, "Test if the remote factory can update its creatable list");
	std::string update_creatables_msg = "update_creatables\n";
	ext_factory_pipe.write(update_creatables_msg.c_str(), static_cast<int>(update_creatables_msg.size()));
	wait_for_event(factory_adp.get(), 1);

	//Check if the remote factory updated its creatable list
	bundle.clear();
	ext_factory->get_creatables(bundle);
	assert(bundle.size() == 2);
	for (auto c : bundle) {
		ZstLog::app(LogLevel::debug, "Creatable: {}", c.path());
	}

	//Check if asking a factory to create a URI that doesn't return anything returns null
	assert(!zst_create_entity(ext_factory->URI() + ZstURI("CustomExternalComponent") + ZstURI("avocado"), "test"));

	//Quit
	if (launch_ext_process) {
		ext_factory_process.terminate();
		ext_factory_log_thread.join();
	}
}


int main(int argc,char **argv)
{
    TestRunner runner("TestEntityFactories", argv[0]);
    test_entity_factories();
	test_remote_factories(argv[0]);
    return 0;
}
