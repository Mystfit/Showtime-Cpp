// Fill out your copyright notice in the Description page of Project Settings.

#include "ShowtimeConversions.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "ShowtimeSubsystem.h"
#include "Kismet/GameplayStatics.h"

TArray<UShowtimeEntity*> ShowtimeConversions::EntityBundleToWrappers(const ZstEntityBundle* bundle)
{
		auto ShowtimeSubsystem = UGameplayStatics::GetGameInstance(GEngine->GetWorld())->GetSubsystem<UShowtimeSubsystem>();
		TArray<UShowtimeEntity*> wrappers;
	
		for (int i = 0; i < bundle->size(); ++i) {
			auto wrapper = ShowtimeSubsystem->View->EntityWrappers.Find(UTF8_TO_TCHAR(bundle->item_at(i)->URI().path()));
			if (wrapper)
				wrappers.Add(*wrapper);
		}
	
		return wrappers;
}
