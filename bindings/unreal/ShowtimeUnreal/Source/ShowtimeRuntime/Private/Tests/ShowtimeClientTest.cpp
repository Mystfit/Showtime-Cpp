#include "Misc/AutomationTest.h"
#include "ShowtimeClient.h"
#include "ShowtimeServer.h"

using namespace showtime;

BEGIN_DEFINE_SPEC(ShowtimeClientSpec, "ShowtimeRuntimeModule.UShowtimeClient", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
UShowtimeClient* client;
UShowtimeServer* server;
END_DEFINE_SPEC(ShowtimeClientSpec)

void ShowtimeClientSpec::Define()
{
	BeforeEach([this]() {
		client = NewObject<UShowtimeClient>();
        server = NewObject<UShowtimeServer>();

        client->Handle()->init("ue4_client", true);
        server->Handle()->init(TCHAR_TO_UTF8(*GetBeautifiedTestName()));
        client->Handle()->auto_join_by_name(TCHAR_TO_UTF8(*GetBeautifiedTestName()));
	});
	
    Describe("init()", [this](){
        It("should return true when successful", [this]()
            {
                TestTrue("is_init_completed", client->Handle()->is_init_completed());
            });
    });

    Describe("auto_join_by_name()", [this]() {
        It("should connect to a broadcasting server", [this]()
            {                
                TestTrue("is_connected", client->Handle()->is_connected());
            });
    });

    Describe("add_log_adaptor()", [this]() {
        LatentIt("should capture log records", [this](const FDoneDelegate& Done)
            {
                // Log::app(Log::Level::debug, "test");
                client->Handle()->poll_once();
                Done.Execute();
                //BackendService->QueryItems(this, &FMyCustomSpec::HandleQueryItemComplete, Done);
            });
    });

	AfterEach([this]() {
		client->Handle()->destroy();
        server->Handle()->destroy();
	});
}

//void ShowtimeClientSpec::HandleLogRecord(FDoneDelegate Done)
//{
//    TestEqual("Items.Num() == 5", Items.Num(), 5);
//    Done.Execute();
//}