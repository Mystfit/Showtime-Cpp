#include "ShowtimeEntity.h"
#include "Engine/GameInstance.h"
#include "ShowtimeSubsystem.h"

void AShowtimeEntity::init(FString entity_path) {
	EntityPath = entity_path;
	OnInitialised.Broadcast();
}

AShowtimeEntity* AShowtimeEntity::GetParent() const
{
	AShowtimeEntity* wrapper = nullptr;
	
	auto ShowtimeSubsystem = GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();
	if (!ShowtimeSubsystem)
		return nullptr;

	if (auto entity = GetNativeEntity()) {
		auto parent_path = entity->URI().parent();
		if (parent_path.is_empty())
			return wrapper;

		if (ZstEntityBase* parent_entity = ShowtimeSubsystem->Handle()->find_entity(parent_path)) {
			wrapper = ShowtimeSubsystem->View->GetWrapper(parent_entity);
		}
	}

	return wrapper;
}

TArray<AShowtimeEntity*> AShowtimeEntity::GetChildren(bool recursive) const
{
	TArray<AShowtimeEntity*> child_wrappers;
	auto ShowtimeSubsystem = GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();

	auto entity = GetNativeEntity();
	if(entity && ShowtimeSubsystem){
		if (!ShowtimeSubsystem->View)
			return child_wrappers;

		auto children = std::make_shared<ZstEntityBundle>();
		entity->get_child_entities(children.get(), false, recursive);
		for (int i = 0; i < children->size(); ++i) {
			auto wrapper = ShowtimeSubsystem->View->EntityWrappers.Find(UTF8_TO_TCHAR(children->item_at(i)->URI().path()));
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
	auto ShowtimeSubsystem = GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();

	if (!ShowtimeSubsystem || EntityPath.IsEmpty())
		return nullptr;
	return  ShowtimeSubsystem->Handle()->find_entity(ZstURI(TCHAR_TO_UTF8(*EntityPath)));
}

void AShowtimeEntity::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AShowtimeEntity, EntityPath);
	DOREPLIFETIME(AShowtimeEntity, OnInitialised);
}
