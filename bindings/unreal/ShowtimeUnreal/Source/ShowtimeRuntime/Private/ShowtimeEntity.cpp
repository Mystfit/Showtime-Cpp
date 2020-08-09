#include "ShowtimeEntity.h"
#include "ShowtimeClient.h"

void AShowtimeEntity::init(UShowtimeClient* owner, FString entity_path) {
	OwningClient = owner;
	EntityPath = entity_path;
}

AShowtimeEntity* AShowtimeEntity::GetParent()
{
	if (!OwningClient)
		return nullptr;

	return OwningClient->GetWrapperParent(this);
}
