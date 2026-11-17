// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ROWPlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UROWAttributeSet;
struct FOnAttributeChangeData;

// Fired whenever health changes. Useful for a health bar widget.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHealthChanged, float, NewHealth, float, MaxHealth);

/**
 * The wizard. Adds an Ability System Component and the ROW attribute set to a normal Character,
 * so enemies can damage the player through Gameplay Effects. Restarts the level on death.
 * Set the wizard Blueprint's parent class to this one.
 */
UCLASS()
class ROWDEVBUILD_API AROWPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AROWPlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMaxHealth() const;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnPlayerHealthChanged OnPlayerHealthChanged;

	// Blueprint hook for death effects (animation, sound, game over screen).
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnDied();

protected:
	virtual void BeginPlay() override;

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void RestartLevel();

	// Seconds between dying and the level restarting.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float RestartDelay = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROWAttributeSet> AttributeSet;

	FTimerHandle RestartTimer;
	bool bIsDead = false;
};