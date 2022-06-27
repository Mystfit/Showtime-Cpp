#include "ShowtimeEntity.h"
#include "Engine/GameInstance.h"
#include "ShowtimeSubsystem.h"
#include "ShowtimeConversions.h"

UShowtimeEntity::UShowtimeEntity(const FObjectInitializer& ObjectIn...) : UActorComponent(ObjectIn){
	UActorComponent::SetIsReplicatedByDefault(true);
}

void UShowtimeEntity::Init(const ZstURI& entityURI) {
	//EntityPath = entity_path;
	//URI = NewObject<UShowtimeURI>();
	//URI->Init(entityURI);
	OnInitialised.Broadcast();
}

UShowtimeEntity* UShowtimeEntity::GetParent() const
{
	UShowtimeEntity* wrapper = nullptr;
	
	auto ShowtimeSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();
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

TArray<UShowtimeEntity*> UShowtimeEntity::GetChildren(bool recursive) const
{
	TArray<UShowtimeEntity*> child_wrappers;
	auto entity = GetNativeEntity();
	if(entity){
		auto children = std::make_shared<ZstEntityBundle>();
		entity->get_child_entities(children.get(), false, recursive);
		child_wrappers = ShowtimeConversions::EntityBundleToWrappers(children.get());
	}

	return child_wrappers;
}

FString UShowtimeEntity::GetName() const
{
	auto entity = GetNativeEntity();
	if (entity) {
		return UTF8_TO_TCHAR(entity->URI().last().path());
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
	auto ShowtimeSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UShowtimeSubsystem>();

	if (!URI || !ShowtimeSubsystem)
		return nullptr;
	return  ShowtimeSubsystem->Handle()->find_entity(URI->Native());
}

void UShowtimeEntity::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(UShowtimeEntity, EntityPath);
	DOREPLIFETIME(UShowtimeEntity, URI);
	DOREPLIFETIME(UShowtimeEntity, OnInitialised);
}
