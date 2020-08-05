#include "ShowtimeComponent.h"
#include "ShowtimeClient.h"

ZstComponent* AShowtimeComponent::GetNative() {
	if (!Owner || EntityPath.IsEmpty())
		return nullptr;

	auto entity = Owner->Handle()->find_entity(ZstURI(TCHAR_TO_UTF8(*EntityPath)));
	
	if (entity->entity_type() == ZstEntityType::COMPONENT) {
		return static_cast<ZstComponent*>(entity);
	}

	return nullptr;
}
