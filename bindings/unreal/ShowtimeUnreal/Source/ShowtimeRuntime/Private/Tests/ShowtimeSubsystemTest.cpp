#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "ShowtimeSubsystem.h"

#include <showtime/ShowtimeServer.h>
#include "ShowtimeServer.h"

using namespace showtime;

BEGIN_DEFINE_SPEC(ShowtimeSubsystemSpec, "ShowtimeRuntimeModule.UShowtimeSubsystem", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
UGameInstance* GameInstance;
UShowtimeSubsystem* subsystem;
UShowtimeServer* server;
END_DEFINE_SPEC(ShowtimeSubsystemSpec)

void ShowtimeSubsystemSpec::Define()
{
	BeforeEach([this]() {
        GameInstance = NewObject<UGameInstance>();
        GameInstance->Init();
        subsystem = GameInstance->GetSubsystem<UShowtimeSubsystem>();
        server = NewObject<UShowtimeServer>();

        subsystem->Handle()->init("ue4_client", true);
        auto servername = GetBeautifiedTestName();
        server->Handle()->init(TCHAR_TO_UTF8(*servername));
        subsystem->Handle()->auto_join_by_name(TCHAR_TO_UTF8(*GetBeautifiedTestName()));
	});
	
    Describe("init()", [this](){
        It("should return true when successful", [this]()
            {
                TestTrue("is_init_completed", subsystem->Handle()->is_init_completed());
            });
    });

    Describe("auto_join_by_name()", [this]() {
        It("should connect to a broadcasting server", [this]()
            {                
                TestTrue("is_connected", subsystem->Handle()->is_connected());
            });
    });

    Describe("add_log_adaptor()", [this]() {
        LatentIt("should capture log records", [this](const FDoneDelegate& Done) 
            {
                std::string log_msg = "test";
                // Log::app(Log::Level::debug, "test");
                //auto log_func = ;
                char* result = nullptr;
                //subsystem->Handle()->log_events()->formatted_log_record()->add([&result](const char* formatted_log_record) {result = (char*)formatted_log_record; });
                subsystem->Handle()->poll_once();
                TestTrue("is_message_logged", strcmp(log_msg.c_str(), result) == 0);
                Done.Execute();
                //BackendService->QueryItems(this, &FMyCustomSpec::HandleQueryItemComplete, Done);
            });
    });

	AfterEach([this]() {
        subsystem->Handle()->destroy();
        server->Handle()->destroy();
	});
}

//void ShowtimeClientSpec::HandleLogRecord(FDoneDelegate Done)
//{
//    TestEqual("Items.Num() == 5", Items.Num(), 5);
//    Done.Execute();
//}