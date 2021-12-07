#include "TestCommon.hpp"

#define BOOST_TEST_MODULE Entities

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
}

BOOST_FIXTURE_TEST_CASE(async_deactivation_callback, FixtureOutputEntity) {
	test_client->get_root()->add_child(output_component.get());
	auto entity_sync_event = std::make_shared<TestSynchronisableEvents>();
	output_component->add_adaptor(entity_sync_event);
	entity_sync_event->reset_num_calls();

	//Deactivate
	test_client->deactivate_entity_async(output_component.get());
	wait_for_event(test_client, entity_sync_event, 1);
	BOOST_TEST(!output_component->is_activated());
	BOOST_TEST(!test_client->find_entity(output_component->URI()));
}

BOOST_FIXTURE_TEST_CASE(plug_children, FixtureJoinServer) {
	auto component = std::make_unique<ZstComponent>("component");
	auto plug = std::make_unique<ZstOutputPlug>("plug", ZstValueType::FloatList);
	
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
	parent->add_child(child.get());
	
	BOOST_TEST(child->is_activated());
	BOOST_TEST(child->URI() == child_path);
	BOOST_TEST(test_client->find_entity(child->URI()));
}

BOOST_FIXTURE_TEST_CASE(get_children, FixtureJoinServer) {
	auto parent = std::make_unique<ZstComponent>("test_parent");
	auto child = std::make_unique<ZstComponent>("test_child");
	test_client->register_entity(parent.get());
	parent->add_child(child.get());
	test_client->get_root()->add_child(parent.get());

	ZstEntityBundle bundle;
	// Get a flat list of all entities including root performer
	test_client->get_root()->get_child_entities(&bundle, true, true);
	auto compare = test_client->get_root()->URI();
	auto entity_it = std::find_if(bundle.begin(), bundle.end(), [&compare](const ZstEntityBase* entity) {
		return compare == entity->URI();
	});
	bool found = (entity_it != bundle.end());
	BOOST_TEST(found);
	bundle.clear();

	// Get a list of all recursive children
	test_client->get_root()->get_child_entities(&bundle, false , true);
	compare = test_client->get_root()->URI();
	entity_it = std::find_if(bundle.begin(), bundle.end(), [&compare](const ZstEntityBase* entity) {
		return compare == entity->URI();
	});
	found = (entity_it != bundle.end());
	BOOST_TEST(!found);
	bundle.clear();

	// Get immediate children only
	parent->get_child_entities(&bundle);
	compare = child->URI();
	entity_it = std::find_if(bundle.begin(), bundle.end(), [&compare](const ZstEntityBase* entity) {
		return compare == entity->URI();
		});
	found = (entity_it != bundle.end());
	BOOST_TEST(found);
	bundle.clear();

	// Only get children of a specified type
	test_client->get_root()->get_child_entities(&bundle, false, true, ZstEntityType::COMPONENT);
	BOOST_TEST(bundle.size() == 2);
	bool type_match = (bundle.item_at(0)->entity_type() == ZstEntityType::COMPONENT);
	BOOST_TEST(type_match);
	type_match = (bundle.item_at(1)->entity_type() == ZstEntityType::COMPONENT);
	BOOST_TEST(type_match);
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


BOOST_FIXTURE_TEST_CASE(performer_activates_children, FixtureInitAndCreateServerWithEpheremalPort) {
	auto parent = std::make_unique<OutputComponent>("test_parent");
	test_client->register_entity(parent.get());
	auto child = std::make_unique<OutputComponent>("test_child");
	parent->add_child(child.get());
	BOOST_TEST(child->is_registered());
	test_client->get_root()->add_child(parent.get());
	test_client->auto_join_by_name(server_name.c_str());

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
	parent->get_child_entities(&bundle, false);
	BOOST_TEST(bundle.size() == 1);
	BOOST_TEST(!parent->walk_child_by_URI(child_URI));
	BOOST_TEST(!test_client->find_entity(child_URI));
}

BOOST_FIXTURE_TEST_CASE(child_activation_callback, FixtureParentChild) {
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

BOOST_FIXTURE_TEST_CASE(deleting_parent_deactivates_child, FixtureParentChild) {
	test_client->get_root()->add_child(parent.get());
	auto parent_URI = parent->URI();
	parent = NULL;
	BOOST_TEST(!test_client->find_entity(parent_URI));
	BOOST_TEST(!test_client->find_entity(child->URI()));
	BOOST_TEST(!child->is_activated());
}

BOOST_FIXTURE_TEST_CASE(deleting_entities, FixtureParentChild) {
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

BOOST_FIXTURE_TEST_CASE(rename_entity, FixtureParentChild) {
	test_client->get_root()->add_child(parent.get());
	auto orig_path = parent->URI();
	parent->set_name("renamed_parent");
	BOOST_TEST(!test_client->find_entity(orig_path));
	BOOST_TEST(!test_client->get_root()->walk_child_by_URI(orig_path));
	BOOST_TEST(test_client->find_entity(test_client->get_root()->URI() + ZstURI("renamed_parent")));
	BOOST_TEST(test_client->find_entity(test_client->get_root()->URI() + ZstURI("renamed_parent/child")));
}

BOOST_FIXTURE_TEST_CASE(rename_performer_before_join, FixtureInit) {
	auto child = std::make_unique<ZstComponent>("child");
	test_client->get_root()->add_child(child.get());
	test_client->get_root()->set_name("renamed_client");
	BOOST_TEST(test_client->get_root()->URI() == ZstURI("renamed_client"));
	BOOST_TEST(test_client->find_entity(ZstURI("renamed_client/child")));
}

BOOST_FIXTURE_TEST_CASE(child_addition_callback, FixtureJoinServer) {
	auto parent = std::make_unique<OutputComponent>("test_parent");
	auto child = std::make_unique<OutputComponent>("test_child");

	bool child_found = false;

	// Test child addition
	parent->entity_events()->child_entity_added()->add([&child_found, child_ent = child.get()](ZstEntityBase* entity) {
		if (entity == child_ent) 
			child_found = true; 
	});

	test_client->get_root()->add_child(parent.get());
	parent->add_child(child.get());
	wait_for_condition(test_client, child_found);
	BOOST_TEST(child_found);
}

BOOST_FIXTURE_TEST_CASE(child_removal_callback, FixtureParentChild) {
	test_client->get_root()->add_child(parent.get());

	bool child_removed = false;
	parent->entity_events()->child_entity_removed()->add([&child_removed, orig_path = child.get()->URI()](const ZstURI& entity_path){
		if (entity_path == orig_path)
			child_removed = true;
	});
	parent->remove_child(child.get());
	wait_for_condition(test_client, child_removed);
	BOOST_TEST(child_removed);
}


BOOST_FIXTURE_TEST_CASE(entity_tick, FixtureJoinServer) {
	auto entity = std::make_unique<OutputComponent>("test_parent");
	test_client->get_root()->add_child(entity.get());
	bool did_tick = false;
	entity->entity_events()->tick() += [&did_tick](ZstEntityBase* entity) { did_tick = true; };

	entity->register_tick();
	test_client->poll_once();
	BOOST_TEST(did_tick);
}
