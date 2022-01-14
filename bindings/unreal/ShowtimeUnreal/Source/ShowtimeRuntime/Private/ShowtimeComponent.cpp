#include "ShowtimeComponent.h"
#include "ShowtimeSubsystem.h"


//void UShowtimeComponent::AttachPlug(UShowtimePlug* plug)
//{
//	if (!plug)
//		return;
//
//	if (auto native_plug = plug->GetNativeEntity()) {
//		if (auto native_component = GetNativeComponent()) {
//			native_component->add_child(native_plug);
//			this->PlugPlaced(plug);
//		}
//	}
//}

//void UShowtimeComponent::AttachComponent(UShowtimeComponent* component)
//{
//	if (!component)
//		return;
//
//	if (auto native_child = component->GetNativeEntity()) {
//		if (auto native_component = GetNativeComponent()) {
//			native_component->add_child(native_child);
//			this->ComponentPlaced(component);
//		}
//	}
//}

void UShowtimeComponent::PlugPlaced_Implementation(UShowtimePlug* plug)
{
	OnPlugPlaced.Broadcast(plug);
}

void UShowtimeComponent::ComponentPlaced_Implementation(UShowtimeComponent* component)
{
	OnComponentPlaced.Broadcast(component);
}

ZstComponent* UShowtimeComponent::GetNativeComponent() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		if (entity->entity_type() == ZstEntityType::COMPONENT) {
			return static_cast<ZstComponent*>(entity);
		}
	}
	return nullptr;
}
