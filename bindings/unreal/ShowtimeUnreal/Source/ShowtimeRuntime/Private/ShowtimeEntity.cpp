#include "ShowtimeEntity.h"
#include "ShowtimeClient.h"

void UShowtimeEntity::init(UShowtimeClient* owner, FString entity_path) {
	OwningClient = owner;
	EntityPath = entity_path;

	OnInitialised.Broadcast();
}

UShowtimeEntity* UShowtimeEntity::GetParent() const
{
	if (!OwningClient)
		return nullptr;

	return OwningClient->GetWrapperParent(this);
}

TArray<UShowtimeEntity*> UShowtimeEntity::GetChildren(bool recursive) const
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

FString UShowtimeEntity::GetName() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		return UTF8_TO_TCHAR(entity->URI().first().path());
	}
	return "";
}

void UShowtimeEntity::AddChild(UShowtimeEntity* entity)
{
	if (auto native_entity = GetNativeEntity()){
		if (auto native_child = entity->GetNativeEntity())
			native_entity->add_child(native_child);
	}
}

ZstEntityBase* UShowtimeEntity::GetNativeEntity() const {
	if (!OwningClient || EntityPath.IsEmpty())
		return nullptr;
	return  OwningClient->Handle()->find_entity(ZstURI(TCHAR_TO_UTF8(*EntityPath)));
}

void UShowtimeEntity::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UShowtimeEntity, EntityPath);
	DOREPLIFETIME(UShowtimeEntity, OnInitialised);
}
