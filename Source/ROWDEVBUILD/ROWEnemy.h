// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ROWEnemy.generated.h"

class UAbilitySystemComponent;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROWAttributeSet> AttributeSet;
};