#include "Misc/AutomationTest.h"
#include "ShowtimeClient.h"
#include "ShowtimeServer.h"

using namespace showtime;

#define TEST_SERVER "test_server"

BEGIN_DEFINE_SPEC(ShowtimeClientSpec, "ShowtimeRuntimeModule.UShowtimeClient", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
UShowtimeClient* client;
UShowtimeServer* server;
END_DEFINE_SPEC(ShowtimeClientSpec)

void ShowtimeClientSpec::Define()
{
	BeforeEach([this]() {
		client = NewObject<UShowtimeClient>();
        server = NewObject<UShowtimeServer>();
        server->init(TEST_SERVER);
	});
	
    Describe("init()", [this](){
        It("should return true when successful", [this]()
            {
                client->init("ue4_client", true);
                TestTrue("Execute", client->is_init_completed());
            });
    });

    Describe("auto_join_by_name()", [this]() {
        It("should connect to a broadcasting server", [this]()
            {
                client->init("ue4_client", true);
                client->auto_join_by_name(TEST_SERVER);
                TestTrue("is_connecteds", client->is_connected());
            });
        });

	AfterEach([this]() {
		client->destroy();
        server->destroy();
	});
}