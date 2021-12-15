#include "ShowtimeCable.h"

#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShowtimeSubsystem.h"


//FString UShowtimeCableAddress::InputPath() const {
//	return FString(UTF8_TO_TCHAR(this->get_input_URI().path()));
//}
//
//FString UShowtimeCableAddress::OutputPath() const {
//	return FString(UTF8_TO_TCHAR(this->get_output_URI().path()));
//}

//FShowtimeCableAddress UShowtimeCable::Address() const
//{
//	auto input = GetInputPlug()->GetNativePlug();
//	auto output = GetOutputPlug()->GetNativePlug();
//	return FShowtimeCableAddress((input) ?input->URI().path() : ZstURI(), (output) ? output->URI().path() : ZstURI());
//}

UShowtimePlug* UShowtimeCable::GetInputPlug() const
{
	auto ShowtimeSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();
	if (auto view = ShowtimeSubsystem->View) {
		auto plug = view->GetWrapper(ZstURI(TCHAR_TO_UTF8(*Address.InputPath)));
		if (plug) {
			if (plug->GetNativeEntity()->entity_type() == ZstEntityType::PLUG)
				return static_cast<UShowtimePlug*>(plug);
		}
	}
	return nullptr;
}

UShowtimePlug* UShowtimeCable::GetOutputPlug() const
{
	auto ShowtimeSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();
	if (auto view = ShowtimeSubsystem->View) {
		auto plug = view->GetWrapper(ZstURI(TCHAR_TO_UTF8(*Address.OutputPath)));
		if (plug) {
			if (plug->GetNativeEntity()->entity_type() == ZstEntityType::PLUG)
				return static_cast<UShowtimePlug*>(plug);
		}
	}
	return nullptr;
}

ZstCable* UShowtimeCable::GetNativeCable() const
{
	auto ShowtimeSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();
	return ShowtimeSubsystem->Handle()->find_cable(CableAddressFromUnreal(Address));
}
