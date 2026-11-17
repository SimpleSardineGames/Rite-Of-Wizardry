// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "ROWEnemy.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UMaterialInterface;
class UROWAttributeSet;
struct FOnAttributeChangeData;

/**
 * Basic enemy. Owns an Ability System Component and the ROW attribute set, so the
 * Fireball can damage it through a Gameplay Effect. Destroys itself at zero health.
 * Chasing is set up in the Blueprint child (BP_Enemy).
 */
UCLASS()
class ROWDEVBUILD_API AROWEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AROWEnemy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealth() const;

protected:
	virtual void BeginPlay() override;

	void OnHealthChanged(const FOnAttributeChangeData& Data);

	// Damages the player if they are within AttackRange. Runs on a repeating timer.
	void TryAttackPlayer();

	// Called when the Status.Slowed tag is added or removed.
	void OnSlowedTagChanged(const FGameplayTag Tag, int32 NewCount);

	// Movement speed multiplier while slowed by Frost.
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", ClampMax = "1"))
	float SlowedSpeedMultiplier = 0.4f;

	// Normal walk speed, saved at BeginPlay so the slow can be undone.
	float BaseWalkSpeed = 0.f;

	// Gameplay Effect applied to the player on contact (for example GE_Damage_Test).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> ContactDamageEffect;

	// How close the player must be to take contact damage.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackRange = 120.f;

	// Seconds between contact damage checks.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackInterval = 1.f;

	FTimerHandle AttackTimer;

	// ---- Hit feedback ----

	// Blueprint hook for extra hit feedback (sound, particles, camera shake).
	UFUNCTION(BlueprintImplementableEvent, Category = "Feedback")
	void OnDamaged(float DamageTaken);

	void PlayHitFeedback();
	void ClearHitFlash();

	// Material drawn over the mesh for a moment when hit (for example M_HitFlash). Empty = no flash.
	UPROPERTY(EditDefaultsOnly, Category = "Feedback")
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	// How long the flash stays on, in seconds.
	UPROPERTY(EditDefaultsOnly, Category = "Feedback", meta = (ClampMin = "0"))
	float HitFlashDuration = 0.1f;

	// Strength of the push away from the player when hit. 0 = no knockback.
	UPROPERTY(EditDefaultsOnly, Category = "Feedback", meta = (ClampMin = "0"))
	float KnockbackStrength = 600.f;

	FTimerHandle HitFlashTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROWAttributeSet> AttributeSet;
};