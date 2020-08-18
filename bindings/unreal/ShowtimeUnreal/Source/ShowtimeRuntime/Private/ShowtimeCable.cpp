#include "ShowtimeCable.h"
#include "ShowtimeClient.h"


//FString UShowtimeCableAddress::InputPath() const {
//	return FString(UTF8_TO_TCHAR(this->get_input_URI().path()));
//}
//
//FString UShowtimeCableAddress::OutputPath() const {
//	return FString(UTF8_TO_TCHAR(this->get_output_URI().path()));
//}

FShowtimeCableAddress AShowtimeCable::Address() const
{
	auto input = GetInputPlug()->GetNativePlug();
	auto output = GetOutputPlug()->GetNativePlug();
	return FShowtimeCableAddress{
		(input) ? UTF8_TO_TCHAR(input->URI().path()) : TEXT(""), 
		(output) ? UTF8_TO_TCHAR(output->URI().path()) : TEXT("")
	};
}

UShowtimePlug* AShowtimeCable::GetInputPlug() const
{
	auto plug = OwningClient->EntityWrappers.Find(UTF8_TO_TCHAR(GetNativeCable()->get_input()->URI().path()));
	if (plug) {
		if ((*plug)->GetNativeEntity()->entity_type() == ZstEntityType::PLUG)
			return static_cast<UShowtimePlug*>(*plug);
	}
	return nullptr;
}

UShowtimePlug* AShowtimeCable::GetOutputPlug() const
{
	auto plug = OwningClient->EntityWrappers.Find(UTF8_TO_TCHAR(GetNativeCable()->get_output()->URI().path()));
	if (plug) {
		if ((*plug)->GetNativeEntity()->entity_type() == ZstEntityType::PLUG)
			return static_cast<UShowtimePlug*>(*plug);
	}
	return nullptr;
}

ZstCable* AShowtimeCable::GetNativeCable() const
{
	auto input = GetInputPlug()->GetNativePlug();
	auto output = GetOutputPlug()->GetNativePlug();
	if(input && output)
		return OwningClient->Handle()->find_cable(input->URI(), output->URI());
	return nullptr;
}
