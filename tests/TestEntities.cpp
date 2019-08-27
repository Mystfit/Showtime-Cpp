#define BOOST_TEST_MODULE Entities

#include "TestCommon.hpp"

using namespace ZstTest;

class FixtureOutputEntity : public FixtureJoinServer {
public:
	FixtureOutputEntity() : output_component(std::make_unique<OutputComponent>("entity_create_test_sync")){}
	~FixtureOutputEntity() {};
	std::unique_ptr<OutputComponent> output_component;
};

class FixtureParentChild : public FixtureJoinServer {
public:
	FixtureParentChild() : 
		parent(std::make_unique<OutputComponent>("parent")),
		child(std::make_unique<OutputComponent>("child")) 
	{
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
		client->get_root()->add_child(component.get());
	}

	~FixtureComponentWithAdaptor()
	{
	}
};

BOOST_FIXTURE_TEST_CASE(activation, FixtureOutputEntity) {
	//Activate
	client->get_root()->add_child(output_component.get());
	BOOST_TEST(output_component->is_activated());
	BOOST_TEST(client->find_entity(output_component->URI()));
	
	//Deactivate
	client->deactivate_entity(output_component.get());
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!client->find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(adaptor_cleanup, FixtureJoinServer) {
	//Test that entities remove themselves from adaptors when they leave
	BOOST_TEST_CHECKPOINT("Deactivating component to make sure that the deleted adaptor has removed itself from the component first");
	{
		auto component = std::make_unique<OutputComponent>("solo");
		{
			auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
			component->add_adaptor(entity_sync_event);
			client->get_root()->add_child(component.get());
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
			client->get_root()->add_child(component.get());
		}
	}
}

BOOST_FIXTURE_TEST_CASE(async_activation_callback, FixtureOutputEntity) {
	auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
	output_component->add_adaptor(entity_sync_event);

	//Activate
	client->get_root()->add_child(output_component.get());
	wait_for_event(client, entity_sync_event, 1);
	BOOST_TEST(output_component->is_activated());
	BOOST_TEST(client->find_entity(output_component->URI()));
	entity_sync_event->reset_num_calls();

	//Deactivate
	client->deactivate_entity_async(output_component.get());
	wait_for_event(client, entity_sync_event, 1);
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!client->find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(plug_children, FixtureOutputEntity) {
	client->get_root()->add_child(output_component.get());
	auto plug_child = output_component->get_child_by_URI(output_component->output()->URI());
	BOOST_TEST(plug_child);
	BOOST_TEST(ZstURI::equal(plug_child->URI(), output_component->output()->URI()));
}

BOOST_FIXTURE_TEST_CASE(add_child, FixtureParentChild) {
	client->get_root()->add_child(parent.get());
	BOOST_TEST(client->find_entity(parent->URI()));
	BOOST_TEST(client->find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(remove_child, FixtureParentChild) {
	auto child_URI = child->URI();
	client->get_root()->add_child(parent.get());
	client->deactivate_entity(child.get());
	BOOST_TEST(!parent->walk_child_by_URI(child_URI));
	BOOST_TEST(!client->find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_activation_callback, FixtureParentChild) {
	client->deactivate_entity(child.get());
	auto child_activation_event = std::make_shared<TestSynchronisableEvents>();
	child->add_adaptor(child_activation_event);
	client->get_root()->add_child(parent.get());

	//Activation
	parent->add_child(child.get());
	wait_for_event(client, child_activation_event, 1);
	BOOST_TEST(child->is_activated());

	//Deactivation
	child_activation_event->reset_num_calls();
	client->deactivate_entity(child.get());
	wait_for_event(client, child_activation_event, 1);
	BOOST_TEST(!child->is_activated());
}

BOOST_FIXTURE_TEST_CASE(parent_deactivates_child, FixtureParentChild) {
	client->get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	client->deactivate_entity(parent.get());
	BOOST_TEST(!client->find_entity(parent->URI()));
	BOOST_TEST(!client->find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(deleting_entities_deactivates, FixtureParentChild) {
	client->get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	auto child_URI = parent->URI();
	child = NULL;
	parent = NULL;
	BOOST_TEST(!client->find_entity(parent_URI));
	BOOST_TEST(!client->find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_deletion_removes_from_parent, FixtureParentChild) {
    client->get_root()->add_child(parent.get());
    auto child_URI = child->URI();
    child = NULL;
    BOOST_TEST(!parent->get_child_by_URI(child_URI));
    BOOST_TEST(!client->find_entity(child_URI));
}
