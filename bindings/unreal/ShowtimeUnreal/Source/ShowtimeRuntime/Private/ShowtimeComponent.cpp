#include "ShowtimeComponent.h"
#include "ShowtimeClient.h"


ZstComponent* UShowtimeComponent::GetNativeComponent()
{
	auto entity = GetNativeEntity();
	if (entity->entity_type() == ZstEntityType::COMPONENT) {
		return static_cast<ZstComponent*>(entity);
	}
	return nullptr;
}
