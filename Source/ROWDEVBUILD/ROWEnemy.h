// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ROWEnemy.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROWAttributeSet> AttributeSet;
};