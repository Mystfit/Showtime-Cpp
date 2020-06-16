#include "Misc/AutomationTest.h"
#include "ShowtimeClient.h"

using namespace showtime;

BEGIN_DEFINE_SPEC(ShowtimeClientSpec, "ShowtimeRuntimeModule.UShowtimeClient", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
UShowtimeClient* client;
END_DEFINE_SPEC(ShowtimeClientSpec)

void ShowtimeClientSpec::Define()
{
	BeforeEach([this]() {
		client = NewObject<UShowtimeClient>();
	});
	
    Describe("init()", [this]()
        {
            It("should return true when successful", [this]()
                {
                    client->init("ue4_client", true);
                    TestTrue("Execute", client->is_init_completed());
                });
        });

	AfterEach([this]() {
		client->destroy();
	});
}