#include "ShowtimeCable.h"
#include "ShowtimeClient.h"


//FString UShowtimeCableAddress::InputPath() const {
//	return FString(UTF8_TO_TCHAR(this->get_input_URI().path()));
//}
//
//FString UShowtimeCableAddress::OutputPath() const {
//	return FString(UTF8_TO_TCHAR(this->get_output_URI().path()));
//}

//FShowtimeCableAddress AShowtimeCable::Address() const
//{
//	auto input = GetInputPlug()->GetNativePlug();
//	auto output = GetOutputPlug()->GetNativePlug();
//	return FShowtimeCableAddress((input) ?input->URI().path() : ZstURI(), (output) ? output->URI().path() : ZstURI());
//}

AShowtimePlug* AShowtimeCable::GetInputPlug() const
{
	auto plug = OwningClient->GetWrapper(ZstURI(TCHAR_TO_UTF8(*Address.InputPath)));
	if (plug) {
		if (plug->GetNativeEntity()->entity_type() == ZstEntityType::PLUG)
			return static_cast<AShowtimePlug*>(plug);
	}
	return nullptr;
}

AShowtimePlug* AShowtimeCable::GetOutputPlug() const
{
	auto plug = OwningClient->GetWrapper(ZstURI(TCHAR_TO_UTF8(*Address.OutputPath)));
	if (plug) {
		if (plug->GetNativeEntity()->entity_type() == ZstEntityType::PLUG)
			return static_cast<AShowtimePlug*>(plug);
	}
	return nullptr;
}

ZstCable* AShowtimeCable::GetNativeCable() const
{
	return OwningClient->Handle()->find_cable(CableAddressFromUnreal(Address));
}
