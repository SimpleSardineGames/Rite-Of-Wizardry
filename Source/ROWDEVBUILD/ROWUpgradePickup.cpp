// Copyright Holden Woods. Rite of Wizardry.

#include "ROWUpgradePickup.h"
#include "ROWPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"

AROWUpgradePickup::AROWUpgradePickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(80.f);
	PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AROWUpgradePickup::OnPickupOverlap);
	SetRootComponent(PickupSphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(PickupSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
	NameText->SetupAttachment(PickupSphere);
	NameText->SetHorizontalAlignment(EHTA_Center);
	NameText->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	NameText->SetWorldSize(35.f);
}

void AROWUpgradePickup::Setup(const FROWUpgrade& InUpgrade, const TArray<AROWUpgradePickup*>& InGroup)
{
	Upgrade = InUpgrade;
	NameText->SetText(Upgrade.Name);

	Group.Reset();
	for (AROWUpgradePickup* Pickup : InGroup)
	{
		if (Pickup && Pickup != this)
		{
			Group.Add(Pickup);
		}
	}

	OnSetup(Upgrade);
}

void AROWUpgradePickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AROWPlayerCharacter* Player = Cast<AROWPlayerCharacter>(OtherActor);
	if (bTaken || !Player)
	{
		return;
	}
	bTaken = true;

	if (UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent())
	{
		if (Upgrade.Effect)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Upgrade.Effect, 1.f, Context);
			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	OnTaken();

	// Pick 1: the other choices vanish.
	for (const TWeakObjectPtr<AROWUpgradePickup>& Other : Group)
	{
		if (Other.IsValid())
		{
			Other->bTaken = true;
			Other->Destroy();
		}
	}

	Destroy();
}