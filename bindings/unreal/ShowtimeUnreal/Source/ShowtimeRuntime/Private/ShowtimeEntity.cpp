#include "ShowtimeEntity.h"
#include "ShowtimeClient.h"

void UShowtimeEntity::init(UShowtimeClient* owner, FString entity_path) {
	OwningClient = owner;
	EntityPath = entity_path;
}

UShowtimeEntity* UShowtimeEntity::GetParent()
{
	if (!OwningClient)
		return nullptr;

	return OwningClient->GetWrapperParent(this);
}

TArray<UShowtimeEntity*> UShowtimeEntity::GetChildren(bool recursive)
{
	TArray<UShowtimeEntity*> child_wrappers;

	auto entity = GetNativeEntity();
	if(entity){
		auto children = std::make_shared<ZstEntityBundle>();
		entity->get_child_entities(children.get(), false, recursive);
		for (int i = 0; i < children->size(); ++i) {
			auto wrapper = OwningClient->EntityWrappers.Find(UTF8_TO_TCHAR(children->item_at(i)->URI().path()));
			if (wrapper)
				child_wrappers.Add(*wrapper);
		}
	}

	return child_wrappers;
}


ZstEntityBase* UShowtimeEntity::GetNativeEntity() {
	if (!OwningClient || EntityPath.IsEmpty())
		return nullptr;
	return  OwningClient->Handle()->find_entity(ZstURI(TCHAR_TO_UTF8(*EntityPath)));
}
