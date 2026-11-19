// Copyright Holden Woods. Rite of Wizardry.

#include "ROWCoinPickup.h"
#include "ROWPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AROWCoinPickup::AROWCoinPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(60.f);
	PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AROWCoinPickup::OnPickupOverlap);
	SetRootComponent(PickupSphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(PickupSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AROWCoinPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AROWPlayerCharacter* Player = Cast<AROWPlayerCharacter>(OtherActor))
	{
		Player->AddCoins(Value);
		OnCollected();
		Destroy();
	}
}