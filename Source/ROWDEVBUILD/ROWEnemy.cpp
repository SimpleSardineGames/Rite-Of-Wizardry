// Copyright Holden Woods. Rite of Wizardry. - EV2

#include "ROWEnemy.h"
#include "AbilitySystemComponent.h"
#include "ROWAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

AROWEnemy::AROWEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UROWAttributeSet>(TEXT("AttributeSet"));

	// Let an AI controller take over automatically, whether placed in the level or spawned.
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
}

UAbilitySystemComponent* AROWEnemy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AROWEnemy::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

void AROWEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROWAttributeSet::GetHealthAttribute())
			.AddUObject(this, &AROWEnemy::OnHealthChanged);
	}
}

void AROWEnemy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange,
			FString::Printf(TEXT("Enemy health: %.0f"), Data.NewValue));
	}

	if (Data.NewValue <= 0.f)
	{
		Destroy();
	}
}