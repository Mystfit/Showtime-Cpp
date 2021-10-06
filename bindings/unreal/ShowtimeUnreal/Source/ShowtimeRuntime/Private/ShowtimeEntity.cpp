#include "ShowtimeEntity.h"
#include "ShowtimeClient.h"

void AShowtimeEntity::init(UShowtimeClient* owner, FString entity_path) {
	OwningClient = owner;
	EntityPath = entity_path;

	OnInitialised.Broadcast();
}

AShowtimeEntity* AShowtimeEntity::GetParent() const
{
	if (!OwningClient)
		return nullptr;

	if (!OwningClient->View)
		return nullptr;

	return OwningClient->View->GetWrapperParent(this);
}

TArray<AShowtimeEntity*> AShowtimeEntity::GetChildren(bool recursive) const
{
	TArray<AShowtimeEntity*> child_wrappers;

	auto entity = GetNativeEntity();
	if(entity && OwningClient){
		if (!OwningClient->View)
			return child_wrappers;

		auto children = std::make_shared<ZstEntityBundle>();
		entity->get_child_entities(children.get(), false, recursive);
		for (int i = 0; i < children->size(); ++i) {
			auto wrapper = OwningClient->View->EntityWrappers.Find(UTF8_TO_TCHAR(children->item_at(i)->URI().path()));
			if (wrapper)
				child_wrappers.Add(*wrapper);
		}
	}

	return child_wrappers;
}

FString AShowtimeEntity::GetName() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		return UTF8_TO_TCHAR(entity->URI().first().path());
	}
	return "";
}

void AShowtimeEntity::AddChild(AShowtimeEntity* entity)
{
	if (auto native_entity = GetNativeEntity()){
		if (auto native_child = entity->GetNativeEntity())
			native_entity->add_child(native_child);
	}
}

ZstEntityBase* AShowtimeEntity::GetNativeEntity() const {
	if (!OwningClient || EntityPath.IsEmpty())
		return nullptr;
	return  OwningClient->Handle()->find_entity(ZstURI(TCHAR_TO_UTF8(*EntityPath)));
}

void AShowtimeEntity::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShowtimeEntity, EntityPath);
	DOREPLIFETIME(AShowtimeEntity, OnInitialised);
}
