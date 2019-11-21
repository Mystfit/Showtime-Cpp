#define BOOST_TEST_MODULE Entity factories

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


struct FixtureLocalFactory : public FixtureJoinServer {
	std::unique_ptr<TestFactory> factory;
	ZstURI creatable_URI;

	FixtureLocalFactory() {
		factory = std::make_unique<TestFactory>("customs");
		test_client->register_factory(factory.get());
		creatable_URI = factory->URI() + ZstURI(CUSTOM_COMPONENT);
	}
};


struct FixtureFactoryClient : public FixtureExternalClient {
	FixtureFactoryClient() :
		FixtureExternalClient("TestHelperExternalFactory")
	{
	}
	~FixtureFactoryClient() {};
};

struct FixtureWaitForFactoryClient : public FixtureJoinServer, FixtureFactoryClient {

	std::shared_ptr<TestPerformerEvents> performerEvents;

	FixtureWaitForFactoryClient() : performerEvents(std::make_shared<TestPerformerEvents>())
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
		external_factory = dynamic_cast<ZstEntityFactory*>(test_client->find_entity(ZstURI("extfactory/external_customs")));
		external_factory->add_adaptor(factoryEvents);
		factoryEvents->reset_num_calls();
	}
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

BOOST_FIXTURE_TEST_CASE(find_external_factory, FixtureWaitForFactoryClient) {
	BOOST_TEST_CHECKPOINT("Factory performer arrived");
	BOOST_TEST(test_client->find_entity(ZstURI("extfactory/external_customs")));
}

BOOST_FIXTURE_TEST_CASE(find_external_creatables, FixtureExternalFactory) {
	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	BOOST_TEST(bundle.size() == 1);
	BOOST_TEST(bundle[0] == external_factory->URI() + ZstURI("CustomExternalComponent"));
}

BOOST_FIXTURE_TEST_CASE(create_entity_from_external_factory, FixtureExternalFactory) {
	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	auto entity = test_client->create_entity(bundle[0], "brand_spanking_new_ext");
	BOOST_TEST_REQUIRE(entity);
	BOOST_TEST(test_client->find_entity(external_factory->URI().first() + ZstURI("brand_spanking_new_ext")));
}

BOOST_FIXTURE_TEST_CASE(create_entity_from_external_factory_async, FixtureExternalFactory) {
	auto created_entity_URI = external_factory->URI().first() + ZstURI("brand_spanking_new_ext_async");
	ZstURIBundle bundle;
	external_factory->get_creatables(bundle);
	test_client->create_entity_async(bundle[0], "brand_spanking_new_ext_async");
	wait_for_event(test_client, factoryEvents, 1);
	BOOST_TEST(factoryEvents->last_created_entity == created_entity_URI);
	BOOST_TEST(test_client->find_entity(created_entity_URI));
}

BOOST_FIXTURE_TEST_CASE(updated_creatables_callback, FixtureExternalFactory)
{
	std::string update_creatables_msg = "update_creatables\n";
	external_process_stdin.write(update_creatables_msg.c_str(), static_cast<int>(update_creatables_msg.size()));
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
