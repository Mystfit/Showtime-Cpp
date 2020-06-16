// Fill out your copyright notice in the Description page of Project Settings.


#include "ShowtimeClient.h"

void UShowtimeClient::Tick(float DeltaTime){
	this->poll_once();
}

TStatId UShowtimeClient::GetStatId() const{
	return UObject::GetStatID();
}
