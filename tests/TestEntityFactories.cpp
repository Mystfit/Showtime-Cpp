#include "TestCommon.hpp"
#include <string>
#include <memory>
#include <entities/ZstEntityFactory.h>

using namespace ZstTest;

class CustomComponent : public ZstComponent 
{
public:
    CustomComponent(const char * name) : ZstComponent("CustomComponent", name) {}
protected:
    void compute(ZstInputPlug * plug) override {
        ZstLog::entity(LogLevel::notification, "Custom component {} received a value", this->URI().path());
    }
};

class TestFactory : public ZstEntityFactory
{
public:
	TestFactory(const char * name) : ZstEntityFactory(name) {}

	virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name) override
	{
		ZstSharedEntity entity = NULL;
		if (creatable_path == ZstURI("CustomComponent")) {
			entity = std::shared_ptr<CustomComponent>(new CustomComponent(name));
		}

		if (entity) {
			ZstEntityFactory::register_entity(entity.get());
		}
		return NULL;
	}
};

void test_entity_factories(){
	//Create the factory that will instantiate our custom entity
	std::shared_ptr<TestFactory> test_factory = std::shared_ptr<TestFactory>(new TestFactory("customs"));
	zst_register_factory(test_factory.get());

	//Create an scoped entity that will be owned by the application (not the Showtime library)
	ZstURI entity_path = ZstURI("TestEntityFactories/customs/CustomComponent");
	{
		ZstSharedEntity entity = ZstSharedEntity(zst_create_entity(entity_path, "brand_spanking_new"));
		assert(entity);
		assert(zst_find_entity(entity_path));
	}

	//Test scoped entity destruction
	assert(!zst_find_entity(entity_path));

}

int main(int argc,char **argv)
{
    TestRunner runner("TestEntityFactories", argv[0]);
    test_entity_factories();
    return 0;
}
