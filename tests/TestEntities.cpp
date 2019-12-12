#define BOOST_TEST_MODULE Entities

#include "TestCommon.hpp"

using namespace ZstTest;

class FixtureOutputEntity : public FixtureJoinServer {
public:
	FixtureOutputEntity() : output_component(std::make_unique<OutputComponent>("entity_create_test_sync"))
	{
		test_client->register_entity(output_component.get());
	}
	~FixtureOutputEntity() {};
	std::unique_ptr<OutputComponent> output_component;
};

class FixtureParentChild : public FixtureJoinServer {
public:
	FixtureParentChild() : 
		parent(std::make_unique<OutputComponent>("parent")),
		child(std::make_unique<OutputComponent>("child")) 
	{
		test_client->register_entity(parent.get());
		parent->add_child(child.get());
	}
	~FixtureParentChild() {};

	std::unique_ptr<OutputComponent> parent;
	std::unique_ptr<OutputComponent> child;
};


class FixtureComponentWithAdaptor : public FixtureJoinServer {
public:
	std::shared_ptr<TestSynchronisableEvents> entity_sync_event;
	std::unique_ptr<OutputComponent> component;

	FixtureComponentWithAdaptor() : 
		entity_sync_event(std::make_shared<TestSynchronisableEvents>()),
		component(std::make_unique<OutputComponent>("solo"))
	{
		component->add_adaptor(entity_sync_event);
		test_client->get_root()->add_child(component.get());
	}

	~FixtureComponentWithAdaptor()
	{
	}
};


BOOST_FIXTURE_TEST_CASE(register_entity, FixtureJoinServer) {
	//Register entity
	auto component = std::make_unique<OutputComponent>("entity_register");
	test_client->register_entity(component.get());
	BOOST_TEST(component->is_registered());
	BOOST_TEST(test_client->find_entity(component->URI()));
}

BOOST_FIXTURE_TEST_CASE(activate_entity, FixtureOutputEntity) {
	test_client->get_root()->add_child(output_component.get(), false);
	output_component->activate();
	BOOST_TEST(output_component->is_activated());
	BOOST_TEST(test_client->find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(deactivate_entity_via_client, FixtureOutputEntity) {
	test_client->get_root()->add_child(output_component.get());
	test_client->deactivate_entity(output_component.get());
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!test_client->find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(deactivate_entity, FixtureOutputEntity) {
	test_client->get_root()->add_child(output_component.get());
	output_component->deactivate();
	BOOST_TEST(!output_component->is_activated());
}

BOOST_FIXTURE_TEST_CASE(adaptor_cleanup, FixtureJoinServer) {
	//Test that entities remove themselves from adaptors when they leave
	BOOST_TEST_CHECKPOINT("Deactivating component to make sure that the deleted adaptor has removed itself from the component first");
	{
		auto component = std::make_unique<OutputComponent>("solo");
		{
			auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
			component->add_adaptor(entity_sync_event);
			test_client->get_root()->add_child(component.get());
		}
	}
}

BOOST_FIXTURE_TEST_CASE(component_cleanup, FixtureJoinServer) {

	BOOST_TEST_CHECKPOINT("Deleting adaptor to make sure the component removed itself from the adaptor first");
	{
		auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
		{
			auto component = std::make_unique<OutputComponent>("solo");
			component->add_adaptor(entity_sync_event);
			test_client->get_root()->add_child(component.get());
		}
	}
}

BOOST_FIXTURE_TEST_CASE(async_activation_callback, FixtureOutputEntity) {
	auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
	output_component->add_adaptor(entity_sync_event);

	//Activate
	test_client->get_root()->add_child(output_component.get());
	wait_for_event(test_client, entity_sync_event, 1);
	BOOST_TEST(output_component->is_activated());
	BOOST_TEST(test_client->find_entity(output_component->URI()));
	entity_sync_event->reset_num_calls();

	//Deactivate
	test_client->deactivate_entity_async(output_component.get());
	wait_for_event(test_client, entity_sync_event, 1);
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!test_client->find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(plug_children, FixtureJoinServer) {
	auto component = std::make_unique<ZstComponent>("component");
	auto plug = std::make_unique<ZstOutputPlug>("plug", ValueList_FloatList);
	
	test_client->register_entity(component.get());
	component->add_child(plug.get());
	test_client->get_root()->add_child(component.get());
	
	BOOST_TEST(plug->is_activated());
	auto plug_child = component->get_child_by_URI(plug->URI());
	BOOST_TEST(plug_child);
	BOOST_TEST(plug_child == plug.get());
}

BOOST_FIXTURE_TEST_CASE(add_child, FixtureJoinServer) {
	auto parent = std::make_unique<OutputComponent>("test_parent");
	auto child = std::make_unique<OutputComponent>("test_child");
	auto child_path = ZstURI("test_performer/test_parent/test_child");
	
	test_client->get_root()->add_child(parent.get());
	BOOST_TEST(parent->is_activated());
	BOOST_TEST(test_client->find_entity(parent->URI()));
	parent->add_child(child.get());
	
	BOOST_TEST(child->is_activated());
	BOOST_TEST(child->URI() == child_path);
	BOOST_TEST(test_client->find_entity(child->URI()));

	ZstEntityBundle bundle;
	test_client->get_root()->get_child_entities(bundle, true);
	BOOST_TEST(bundle.size() == 5);
}

BOOST_FIXTURE_TEST_CASE(parent_activates_child, FixtureJoinServer) {
	auto parent = std::make_unique<OutputComponent>("test_parent");
	test_client->register_entity(parent.get());

	auto child = std::make_unique<OutputComponent>("test_child");
	parent->add_child(child.get());
	BOOST_TEST(child->is_registered());

	test_client->get_root()->add_child(parent.get());
	BOOST_TEST(parent->is_activated());
	BOOST_TEST(child->is_activated());
	BOOST_TEST(test_client->find_entity(parent->URI()));
	BOOST_TEST(test_client->find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(remove_child, FixtureParentChild) {
	auto child_URI = child->URI();
	test_client->get_root()->add_child(parent.get());
	test_client->deactivate_entity(child.get());
	
	ZstEntityBundle bundle;
	parent->get_child_entities(bundle, false);
	BOOST_TEST(bundle.size() == 1);
	BOOST_TEST(!parent->walk_child_by_URI(child_URI));
	BOOST_TEST(!test_client->find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_activation_callback, FixtureParentChild) {
	test_client->deactivate_entity(child.get());
	auto child_activation_event = std::make_shared<TestSynchronisableEvents>();
	child->add_adaptor(child_activation_event);
	test_client->get_root()->add_child(parent.get());

	//Activation
	parent->add_child(child.get());
	wait_for_event(test_client, child_activation_event, 1);
	BOOST_TEST(child->is_activated());

	//Deactivation
	child_activation_event->reset_num_calls();
	test_client->deactivate_entity(child.get());
	wait_for_event(test_client, child_activation_event, 1);
	BOOST_TEST(!child->is_activated());
}

BOOST_FIXTURE_TEST_CASE(parent_deactivates_child, FixtureParentChild) {
	test_client->get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	test_client->deactivate_entity(parent.get());
	BOOST_TEST(!test_client->find_entity(parent->URI()));
	BOOST_TEST(!test_client->find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(deleting_entities_deactivates, FixtureParentChild) {
	test_client->get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	auto child_URI = parent->URI();
	child = NULL;
	parent = NULL;
	BOOST_TEST(!test_client->find_entity(parent_URI));
	BOOST_TEST(!test_client->find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_deletion_removes_from_parent, FixtureParentChild) {
    test_client->get_root()->add_child(parent.get());
    auto child_URI = child->URI();
    child = NULL;
    BOOST_TEST(!parent->get_child_by_URI(child_URI));
    BOOST_TEST(!test_client->find_entity(child_URI));
}
