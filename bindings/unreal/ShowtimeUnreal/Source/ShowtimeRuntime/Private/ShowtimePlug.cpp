#include "ShowtimePlug.h"

UShowtimePlugDirection AShowtimePlug::Direction() const {
	auto plug = GetNativePlug();
	if (plug) {
		return ZstPlugDirection_to_ShowtimePlugDirection[plug->direction()];
	}
	return UShowtimePlugDirection::Direction_None;
}


UShowtimeValueType AShowtimePlug::ValueType() const
{
	auto plug = GetNativePlug();
	if (plug) {
		return ZstValueType_to_UShowtimeValueType[plug->get_default_type()];
	}
	return UShowtimeValueType::ValueType_None;
}

void AShowtimePlug::SetIntValues(TArray<int> values)
{
	if (auto plug = GetNativePlug()) {
		for (auto val : values) {
			plug->append_int(val);
		}
	}
}

void AShowtimePlug::SetFloatValues(TArray<float> values)
{
	if (auto plug = GetNativePlug()) {
		for (auto val : values) {
			plug->append_float(val);
		}
	}
}

void AShowtimePlug::SetStringValues(TArray<FString> values)
{
	if (auto plug = GetNativePlug()) {
		for (auto val : values) {
			plug->append_string(TCHAR_TO_UTF8(*val), val.Len());
		}
	}
}

void AShowtimePlug::Fire()
{
	if (auto plug = GetNativePlug()) {
		if(plug->direction() == ZstPlugDirection::OUT_JACK)
			static_cast<ZstOutputPlug*>(plug)->fire();
	}
}

ZstPlug* AShowtimePlug::GetNativePlug() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		if (entity->entity_type() == ZstEntityType::PLUG) {
			return static_cast<ZstPlug*>(entity);
		}
	}
	return nullptr;
}
