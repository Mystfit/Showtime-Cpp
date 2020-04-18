#include "TestCommon.hpp"

#define BOOST_TEST_MODULE Entity factories

#include <string>
#include <memory>

using namespace ZstTest;

#define CUSTOM_COMPONENT "CustomComponent"

class CustomComponent : public ZstComponent 
{
public:
	std::unique_ptr<ZstInputPlug> input;

    CustomComponent(const char * name) : 
		ZstComponent(CUSTOM_COMPONENT, name),
		input(std::make_unique <ZstInputPlug>("input", ZstValueType::IntList)) {}

	virtual void on_registered() override {
		add_child(input.get());
	}

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

			//TODO: Factory created entity leakages
			ZstLog::entity(LogLevel::error, "New factory created entity will leak - replace with smart pointer!");
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


class TestFactoryCreationAdaptor : public ZstHierarchyAdaptor, public TestAdaptor
{
public:
	ZstURI last_arrived_factory;
	ZstURI last_left_factory;

	void on_factory_arriving(ZstEntityFactory* factory) override
	{
		ZstLog::app(LogLevel::debug, "FACTORY_ARRIVING: {}", factory->URI().path());
		last_arrived_factory = factory->URI();
		inc_calls();
	}

	void on_factory_leaving(ZstEntityFactory* factory) override
	{
		ZstLog::app(LogLevel::debug, "FACTORY_LEAVING: {}", factory->URI().path());
		last_left_factory = factory->URI();
		inc_calls();
	}
};



struct FixtureLocalFactory : public FixtureJoinServer {
	std::unique_ptr<TestFactory> factory;
	ZstURI creatable_URI;

	FixtureLocalFactory() {
		factory = std::make_unique<TestFactory>("customs");
		test_client->register_factory(factory.get());
		creatable_URI = factory->URI() + ZstURI(CUSTOM_COMPONENT);
	}
};


struct FixtureFactoryClient : public FixtureRemoteClient {

	std::unique_ptr<TestFactory> ext_factory;

	FixtureFactoryClient(std::string server_name) : 
		FixtureRemoteClient("extfactory", server_name),
		ext_factory(std::make_unique<TestFactory>("external_customs"))
	{
		remote_client->register_factory(ext_factory.get());
	}

	~FixtureFactoryClient() {};
};

struct FixtureWaitForFactoryClient : public FixtureJoinServer, FixtureFactoryClient {

	std::shared_ptr<TestPerformerEvents> performerEvents;

	FixtureWaitForFactoryClient() : 
		FixtureJoinServer(),
		FixtureFactoryClient(server_name),
		performerEvents(std::make_shared<TestPerformerEvents>())
	{
		test_client->add_hierarchy_adaptor(performerEvents);
		BOOST_TEST_CHECKPOINT("Waiting for external client performer to arrive");
		wait_for_event(test_client, performerEvents, 1);
		performerEvents->reset_num_calls();
	}
	~FixtureWaitForFactoryClient() {}
};


struct FixtureExternalFactory : public FixtureWaitForFactoryClient {
	ZstEntityFactory* external_factory;
	std::shared_ptr<TestFactoryAdaptor> factoryEvents;

	FixtureExternalFactory() : factoryEvents(std::make_shared<TestFactoryAdaptor>()){
		auto factory_path = ZstURI("extfactory/external_customs");
		auto factory_events = std::make_shared<TestFactoryCreationAdaptor>();
		test_client->add_hierarchy_adaptor(factory_events);
		wait_for_event(test_client, factory_events, 1);

		external_factory = dynamic_cast<ZstEntityFactory*>(test_client->find_entity(factory_path));
		external_factory->add_adaptor(factoryEvents);
		factoryEvents->reset_num_calls();
	}
};


struct FixtureExternalFactoryEventLoop : public FixtureExternalFactory {
public:
	FixtureExternalFactoryEventLoop() : m_event_loop(boost::bind(&FixtureExternalFactoryEventLoop::event_loop, this)){}

	~FixtureExternalFactoryEventLoop() {
		m_event_loop.interrupt();
		m_event_loop.join();
	}

	void event_loop() {
		while (true) {
			try {
				boost::this_thread::interruption_point();
				this->remote_client->poll_once();
			}
			catch (boost::thread_interrupted) {
				break;
			}
		}
	}
private:
	boost::thread m_event_loop;
};


BOOST_FIXTURE_TEST_CASE(create_factory, FixtureJoinServer){
	std::unique_ptr<TestFactory> factory = std::make_unique<TestFactory>("customs");
	test_client->register_factory(factory.get());
	BOOST_TEST(factory->is_activated());
	ZstEntityFactoryBundle bundle;
	test_client->get_root()->get_factories(bundle);
	BOOST_TEST(bundle.size() == 1);
	BOOST_TEST(bundle[0]->URI() == factory->URI());
}

BOOST_FIXTURE_TEST_CASE(destroy_factory, FixtureLocalFactory) {
	TestFactory* factory = new TestFactory("customs");
	test_client->register_factory(factory);
	delete factory;
	ZstEntityFactoryBundle bundle;
	test_client->get_root()->get_factories(bundle);
	BOOST_TEST(bundle.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(remove_factory, FixtureLocalFactory) {
	test_client->get_root()->remove_factory(factory.get());
	BOOST_TEST(factory->is_activated());
	ZstEntityFactoryBundle bundle;
	test_client->get_root()->get_factories(bundle);
	BOOST_TEST(bundle.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(query_factory_creatables, FixtureLocalFactory) {
	ZstURIBundle bundle;
	factory->get_creatables(bundle);
	BOOST_TEST(bundle.size() == 1);
	BOOST_TEST(bundle[0] == creatable_URI);
}

BOOST_FIXTURE_TEST_CASE(create_entity_from_local_factory, FixtureLocalFactory) {
	auto created_entity_URI = test_client->get_root()->URI() + ZstURI("brand_spanking_new");
	ZstURIBundle bundle;
	factory->get_creatables(bundle);
	{
		auto entity = ZstSharedEntity(test_client->create_entity(bundle[0], "brand_spanking_new"));
		BOOST_TEST(entity);
		BOOST_TEST(test_client->find_entity(created_entity_URI));
		BOOST_TEST_MESSAGE(fmt::format("Entity {} leaving scope", created_entity_URI.path()));
	}
	BOOST_TEST(!test_client->find_entity(created_entity_URI));
}

BOOST_FIXTURE_TEST_CASE(find_external_creatables, FixtureExternalFactory) {
	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	BOOST_TEST(bundle.size() == 1);
	BOOST_TEST(bundle[0] == external_factory->URI() + ZstURI("CustomComponent"));
}

BOOST_FIXTURE_TEST_CASE(create_entity_from_external_factory, FixtureExternalFactoryEventLoop) {
	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	auto client = remote_client.get();
	auto entity = test_client->create_entity(bundle[0], "brand_spanking_new_ext");
	BOOST_TEST_REQUIRE(entity);
	BOOST_TEST(test_client->find_entity(external_factory->URI().first() + ZstURI("brand_spanking_new_ext")));
}

BOOST_FIXTURE_TEST_CASE(create_entity_from_external_factory_async, FixtureExternalFactory) {
	auto created_entity_URI = external_factory->URI().first() + ZstURI("brand_spanking_new_ext_async");
	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	test_client->create_entity_async(bundle[0], "brand_spanking_new_ext_async");
	TAKE_A_BREATH
	remote_client->poll_once();
	wait_for_event(test_client, factoryEvents, 1);
	BOOST_TEST(factoryEvents->last_created_entity == created_entity_URI);
	BOOST_TEST(test_client->find_entity(created_entity_URI));
}

BOOST_FIXTURE_TEST_CASE(updated_creatables_callback, FixtureExternalFactory)
{
	ext_factory->add_creatable(ZstURI("avocado"));
	ext_factory->update_creatables();

	wait_for_event(test_client, factoryEvents, 1);

	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	BOOST_TEST(bundle.size() == 2);
	bool found_avocado = false;
	for (auto c : bundle) {
		if (c == external_factory->URI() + ZstURI("avocado"))
			found_avocado = true;
	}
	BOOST_TEST(found_avocado);
}

BOOST_FIXTURE_TEST_CASE(bad_creatable_path_returns_null, FixtureExternalFactory) {
	BOOST_TEST(!test_client->create_entity(external_factory->URI() + ZstURI("bad_creatable_path"), "test"));
}
