// Copyright Holden Woods. Rite of Wizardry.

#include "ROWEnemy.h"
#include "AbilitySystemComponent.h"
#include "ROWAttributeSet.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

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

	GetWorldTimerManager().SetTimer(AttackTimer, this, &AROWEnemy::TryAttackPlayer, AttackInterval, true);
}

void AROWEnemy::TryAttackPlayer()
{
	if (!ContactDamageEffect)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn || FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation()) > AttackRange)
	{
		return;
	}

	// The player takes damage only if it has an Ability System Component.
	if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(PlayerPawn))
	{
		if (UAbilitySystemComponent* TargetASC = TargetInterface->GetAbilitySystemComponent())
		{
			FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
			Context.AddSourceObject(this);

			const FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(ContactDamageEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
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