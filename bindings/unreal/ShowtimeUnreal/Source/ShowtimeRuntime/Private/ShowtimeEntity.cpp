#include "ShowtimeEntity.h"
#include "ShowtimeClient.h"

void AShowtimeEntity::init(UShowtimeClient* owner, FString entity_path) {
	Owner = owner;
	EntityPath = entity_path;
}