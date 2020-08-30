#include "ShowtimeFactory.h"

void UShowtimeFactory::AddSpawnableComponent(AActor* spawnable_actor)
{
	auto factory = GetNativeFactory();
	if (!factory)
		return;

	auto name = spawnable_actor->GetName();
	ZstURI creatable(TCHAR_TO_UTF8(*name));

	std::shared_ptr<ZstURIBundle> bundle = std::make_shared<ZstURIBundle>();

	//factory->add_creatable();

	// Get updated creatable path
	factory->get_creatables(bundle.get());
	creatable = bundle->item_at(bundle->size()-1);

	SpawnableComponents.Add(FString(UTF8_TO_TCHAR(creatable.path())), spawnable_actor);
}

ZstEntityFactory* UShowtimeFactory::GetNativeFactory() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		if (entity->entity_type() == ZstEntityType::FACTORY) {
			return static_cast<ZstEntityFactory*>(entity);
		}
	}
	return nullptr;
}
