// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ROWPlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UROWAttributeSet;
class UUserWidget;
class AROWFireball;
struct FOnAttributeChangeData;

// Fired whenever health changes. Useful for a health bar widget.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHealthChanged, float, NewHealth, float, MaxHealth);

// Fired whenever the coin count changes. Useful for a coin counter widget.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerCoinsChanged, int32, NewCoins);

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

	// ---- Coins ----

	UFUNCTION(BlueprintPure, Category = "Coins")
	int32 GetCoins() const { return Coins; }

	UFUNCTION(BlueprintCallable, Category = "Coins")
	void AddCoins(int32 Amount);

	// Removes coins if the player has enough. Returns false (and spends nothing) if not.
	UFUNCTION(BlueprintCallable, Category = "Coins")
	bool SpendCoins(int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Coins")
	FOnPlayerCoinsChanged OnPlayerCoinsChanged;

	// ---- Mana and casting ----

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetMana() const;

	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetMaxMana() const;

	// Casts a spell toward Direction if there's enough mana for it (the spell's Mana Cost).
	// Handles the spawn position, the caster as Owner, and spending the mana. Returns true if cast.
	UFUNCTION(BlueprintCallable, Category = "Mana")
	bool TryCastSpell(TSubclassOf<AROWFireball> SpellClass, FVector Direction);

	// ---- Interaction ----

	// Talks to a shopkeeper the player is standing next to. Bind to the Interact key (E).
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

protected:
	virtual void BeginPlay() override;

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	// Shows the game over screen if one is set, otherwise restarts the level.
	void RestartLevel();

	// Turns the body into a physics ragdoll. Needs a Physics Asset on the mesh.
	void StartRagdoll();

	// Collapse into a ragdoll when health reaches 0.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bRagdollOnDeath = true;

	// Strength of the push when the body falls. 0 = just collapse.
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0", EditCondition = "bRagdollOnDeath"))
	float DeathImpulse = 300.f;

	// Seconds between dying and the game over screen (or restart) appearing.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float RestartDelay = 2.f;

	// Death screen (for example WBP_GameOver). Empty = restart the level automatically instead.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<UUserWidget> GameOverWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROWAttributeSet> AttributeSet;

	FTimerHandle RestartTimer;
	bool bIsDead = false;

	// Coins the player starts a run with.
	UPROPERTY(EditDefaultsOnly, Category = "Coins", meta = (ClampMin = "0"))
	int32 StartingCoins = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Coins")
	int32 Coins = 0;

	// Mana restored per second.
	UPROPERTY(EditDefaultsOnly, Category = "Mana", meta = (ClampMin = "0"))
	float ManaRegenPerSecond = 8.f;

	// Spells start this far in front of the wizard.
	UPROPERTY(EditDefaultsOnly, Category = "Mana", meta = (ClampMin = "0"))
	float SpellSpawnDistance = 100.f;

	// Blueprint hook: tried to cast without enough mana (play a fizzle sound, flash the mana bar).
	UFUNCTION(BlueprintImplementableEvent, Category = "Mana")
	void OnOutOfMana();

	void RegenMana();

	FTimerHandle ManaRegenTimer;

	static constexpr float ManaRegenInterval = 0.1f;
};