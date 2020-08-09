#include "ShowtimeComponent.h"
#include "ShowtimeClient.h"


ZstComponent* AShowtimeComponent::GetNative() {
	if (!OwningClient || EntityPath.IsEmpty())
		return nullptr;

	auto entity = OwningClient->Handle()->find_entity(ZstURI(TCHAR_TO_UTF8(*EntityPath)));
	
	if (entity->entity_type() == ZstEntityType::COMPONENT) {
		return static_cast<ZstComponent*>(entity);
	}

	return nullptr;
}
