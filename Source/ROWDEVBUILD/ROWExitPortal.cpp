// Copyright Holden Woods. Rite of Wizardry.

#include "ROWExitPortal.h"
#include "ROWFloorGenerator.h"
#include "ROWPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AROWExitPortal::AROWExitPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->InitSphereRadius(120.f);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AROWExitPortal::OnPortalOverlap);
	SetRootComponent(Trigger);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AROWExitPortal::OnPortalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bUsed || !Cast<AROWPlayerCharacter>(OtherActor))
	{
		return;
	}

	if (AROWFloorGenerator* Generator = Cast<AROWFloorGenerator>(GetOwner()))
	{
		bUsed = true;
		OnPortalUsed();

		if (bLeadsToShop)
		{
			Generator->TravelToShop();
		}
		else
		{
			Generator->StartNextFloor();
		}
	}
}