// Copyright Holden Woods. Rite of Wizardry.

#include "ROWEnemy.h"
#include "AbilitySystemComponent.h"
#include "ROWAttributeSet.h"
#include "ROWGameplayTags.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInterface.h"
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

float AROWEnemy::GetMaxHealth() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

void AROWEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Per-enemy health (normal vs elite). Set before listening so it doesn't count as a hit.
		AbilitySystemComponent->SetNumericAttributeBase(UROWAttributeSet::GetMaxHealthAttribute(), StartingHealth);
		AbilitySystemComponent->SetNumericAttributeBase(UROWAttributeSet::GetHealthAttribute(), StartingHealth);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROWAttributeSet::GetHealthAttribute())
			.AddUObject(this, &AROWEnemy::OnHealthChanged);
	}

	GetWorldTimerManager().SetTimer(AttackTimer, this, &AROWEnemy::TryAttackPlayer, AttackInterval, true);

	// Listen for Frost's slow tag being added or removed.
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(ROWTags::Status_Slowed, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AROWEnemy::OnSlowedTagChanged);
	}
}

void AROWEnemy::OnSlowedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	GetCharacterMovement()->MaxWalkSpeed = NewCount > 0 ? BaseWalkSpeed * SlowedSpeedMultiplier : BaseWalkSpeed;
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
	if (Data.NewValue <= 0.f)
	{
		if (!bIsDead)
		{
			bIsDead = true;
			DropLoot();
			Destroy();
		}
		return;
	}

	// Only react to damage, not healing.
	if (Data.NewValue < Data.OldValue)
	{
		PlayHitFeedback();
		OnDamaged(Data.OldValue - Data.NewValue);
	}
}

void AROWEnemy::PlayHitFeedback()
{
	// Flash: draw the flash material over the mesh briefly.
	if (!HitFlashMaterial && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("%s has no Hit Flash Material set"), *GetClass()->GetName()));
	}

	if (HitFlashMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(HitFlashMaterial);
		GetWorldTimerManager().SetTimer(HitFlashTimer, this, &AROWEnemy::ClearHitFlash, HitFlashDuration, false);
	}

	// Knockback: push the enemy directly away from the player.
	if (KnockbackStrength > 0.f)
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FVector Away = (GetActorLocation() - PlayerPawn->GetActorLocation()).GetSafeNormal2D();
			LaunchCharacter(Away * KnockbackStrength, true, false);
		}
	}
}

void AROWEnemy::DropLoot()
{
	if (!CoinPickupClass || CoinsToDrop <= 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Drop at floor level rather than at the capsule's middle.
	const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Base = GetActorLocation() - FVector(0.f, 0.f, HalfHeight - 20.f);

	for (int32 i = 0; i < CoinsToDrop; ++i)
	{
		const FVector2D Offset = FMath::RandPointInCircle(CoinScatterRadius);
		GetWorld()->SpawnActor<AActor>(CoinPickupClass, Base + FVector(Offset.X, Offset.Y, 0.f), FRotator::ZeroRotator, Params);
	}
}

void AROWEnemy::ClearHitFlash()
{
	if (GetMesh())
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}
}