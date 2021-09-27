#include "ShowtimeComponent.h"
#include "ShowtimeClient.h"


void AShowtimeComponent::AttachPlug(AShowtimePlug* plug)
{
	if (!plug)
		return;

	if (auto native_plug = plug->GetNativeEntity()) {
		if (auto native_component = GetNativeComponent()) {
			native_component->add_child(native_plug);
			this->PlugAttached(plug);
		}
	}
}

void AShowtimeComponent::AttachComponent(AShowtimeComponent* component)
{
	if (!component)
		return;

	if (auto native_child = component->GetNativeEntity()) {
		if (auto native_component = GetNativeComponent()) {
			native_component->add_child(native_child);
			this->ComponentAttached(component);
		}
	}
}

ZstComponent* AShowtimeComponent::GetNativeComponent() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		if (entity->entity_type() == ZstEntityType::COMPONENT) {
			return static_cast<ZstComponent*>(entity);
		}
	}
	return nullptr;
}
