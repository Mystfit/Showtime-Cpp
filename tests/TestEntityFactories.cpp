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

void test_remote_factories()
{

}


int main(int argc,char **argv)
{
    TestRunner runner("TestEntityFactories", argv[0], true, false);
    test_entity_factories();
	test_remote_factories();
    return 0;
}
