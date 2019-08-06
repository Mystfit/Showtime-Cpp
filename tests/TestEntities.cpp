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
	TestSynchronisableEvents * entity_sync_event;
	OutputComponent* component;

	FixtureComponentWithAdaptor()
	{
		entity_sync_event = new TestSynchronisableEvents();
		component = new OutputComponent("solo");
		component->add_adaptor(entity_sync_event);
		zst_get_root()->add_child(component);
	}

	~FixtureComponentWithAdaptor()
	{
		if (component) delete component;
		if (entity_sync_event) delete entity_sync_event;
	}
};

BOOST_FIXTURE_TEST_CASE(activation, FixtureOutputEntity) {
	//Activate
	zst_get_root()->add_child(output_component.get());
	BOOST_TEST(output_component->is_activated());
	BOOST_TEST(zst_find_entity(output_component->URI()));
	
	//Deactivate
	zst_deactivate_entity(output_component.get());
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!zst_find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(adaptor_cleanup, FixtureComponentWithAdaptor) {
	delete entity_sync_event;
	entity_sync_event = NULL;
	BOOST_TEST_CHECKPOINT("Deactivating component to make sure that the deleted adaptor has removed itself from the component first");
	zst_deactivate_entity(component);
	delete component;
	component = NULL;
}

BOOST_FIXTURE_TEST_CASE(component_cleanup, FixtureComponentWithAdaptor) {
	delete component;
	component = NULL;
	BOOST_TEST_CHECKPOINT("Deleting adaptor to make sure the component removed itself from the adaptor first");
	BOOST_TEST(!entity_sync_event->is_target_dispatcher_active());
	delete entity_sync_event;
	entity_sync_event = NULL;
}

BOOST_FIXTURE_TEST_CASE(async_activation_callback, FixtureOutputEntity) {
	auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
	output_component->add_adaptor(entity_sync_event.get());

	//Activate
	zst_get_root()->add_child(output_component.get());
	wait_for_event(entity_sync_event.get(), 1);
	BOOST_TEST(output_component->is_activated());
	BOOST_TEST(zst_find_entity(output_component->URI()));
	entity_sync_event->reset_num_calls();

	//Deactivate
	zst_deactivate_entity_async(output_component.get());
	wait_for_event(entity_sync_event.get(), 1);
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!zst_find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(plug_children, FixtureOutputEntity) {
	zst_get_root()->add_child(output_component.get());
	auto plug_child = output_component->get_child_by_URI(output_component->output()->URI());
	BOOST_TEST(plug_child);
	BOOST_TEST(ZstURI::equal(plug_child->URI(), output_component->output()->URI()));
}

BOOST_FIXTURE_TEST_CASE(add_child, FixtureParentChild) {
	zst_get_root()->add_child(parent.get());
	BOOST_TEST(zst_find_entity(parent->URI()));
	BOOST_TEST(zst_find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(remove_child, FixtureParentChild) {
	auto child_URI = child->URI();
	zst_get_root()->add_child(parent.get());
	zst_deactivate_entity(child.get());
	BOOST_TEST(!parent->walk_child_by_URI(child_URI));
	BOOST_TEST(!zst_find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_activation_callback, FixtureParentChild) {
	zst_deactivate_entity(child.get());
	auto child_activation_event = std::make_shared<TestSynchronisableEvents>();
	child->add_adaptor(child_activation_event.get());
	zst_get_root()->add_child(parent.get());

	//Activation
	parent->add_child(child.get());
	wait_for_event(child_activation_event.get(), 1);
	BOOST_TEST(child->is_activated());

	//Deactivation
	child_activation_event->reset_num_calls();
	zst_deactivate_entity(child.get());
	wait_for_event(child_activation_event.get(), 1);
	BOOST_TEST(!child->is_activated());
}

BOOST_FIXTURE_TEST_CASE(parent_deactivates_child, FixtureParentChild) {
	zst_get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	zst_deactivate_entity(parent.get());
	BOOST_TEST(!zst_find_entity(parent->URI()));
	BOOST_TEST(!zst_find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(deleting_entities_deactivates, FixtureParentChild) {
	zst_get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	auto child_URI = parent->URI();
	child = NULL;
	parent = NULL;
	BOOST_TEST(!zst_find_entity(parent_URI));
	BOOST_TEST(!zst_find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_deletion_removes_from_parent, FixtureParentChild) {
    zst_get_root()->add_child(parent.get());
    auto child_URI = child->URI();
    child = NULL;
    BOOST_TEST(!parent->get_child_by_URI(child_URI));
    BOOST_TEST(!zst_find_entity(child_URI));
}
