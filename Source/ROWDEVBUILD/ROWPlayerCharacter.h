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
class UTexture2D;
struct FOnAttributeChangeData;

// Fired whenever health changes. Useful for a health bar widget.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerHealthChanged, float, NewHealth, float, MaxHealth);

// Fired whenever the coin count changes. Useful for a coin counter widget.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerCoinsChanged, int32, NewCoins);

// Fired when the player switches spells. Useful for a sound or a spell-bar animation.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpellSelected, int32, NewIndex);

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

	// The delay to wait between casts: BaseDelay divided by the CastSpeed attribute.
	// Plug your FireDelay in and use the result for the cooldown Delay node.
	UFUNCTION(BlueprintPure, Category = "Mana")
	float GetCastDelay(float BaseDelay) const;

	// Casts a spell toward Direction if there's enough mana for it (the spell's Mana Cost).
	// Handles the spawn position, the caster as Owner, and spending the mana. Returns true if cast.
	UFUNCTION(BlueprintCallable, Category = "Mana")
	bool TryCastSpell(TSubclassOf<AROWFireball> SpellClass, FVector Direction);

	// ---- Spell slots ----

	// Makes slot Index (0 = first) the active spell. Bind the 1/2/3 keys to this.
	UFUNCTION(BlueprintCallable, Category = "Spells")
	void SelectSpell(int32 Index);

	// Moves to the next (+1) or previous (-1) spell, wrapping around. Good for Q/E or the mouse wheel.
	UFUNCTION(BlueprintCallable, Category = "Spells")
	void CycleSpell(int32 Step);

	// Casts the active spell toward Direction (uses mana). Replaces Try Cast Spell in the firing graph.
	UFUNCTION(BlueprintCallable, Category = "Spells")
	bool TryCastSelectedSpell(FVector Direction);

	UFUNCTION(BlueprintPure, Category = "Spells")
	int32 GetSelectedSpellIndex() const { return SelectedSpellIndex; }

	UFUNCTION(BlueprintPure, Category = "Spells")
	int32 GetSpellSlotCount() const { return SpellSlots.Num(); }

	// Name, icon and mana cost of the spell in slot Index. Returns false if the slot is empty.
	UFUNCTION(BlueprintPure, Category = "Spells")
	bool GetSpellSlotInfo(int32 Index, FText& Name, UTexture2D*& Icon, float& ManaCost) const;

	// True if there's enough mana right now to cast the spell in slot Index.
	UFUNCTION(BlueprintPure, Category = "Spells")
	bool CanAffordSpell(int32 Index) const;

	UPROPERTY(BlueprintAssignable, Category = "Spells")
	FOnSpellSelected OnSpellSelected;

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

	// Spells start this far in front of the wizard.
	UPROPERTY(EditDefaultsOnly, Category = "Mana", meta = (ClampMin = "0"))
	float SpellSpawnDistance = 100.f;

	// Blueprint hook: tried to cast without enough mana (play a fizzle sound, flash the mana bar).
	UFUNCTION(BlueprintImplementableEvent, Category = "Mana")
	void OnOutOfMana();

	void RegenMana();

	// The spells on the spell bar, in order (for example BP_Fireball, BP_Frost, BP_Void).
	UPROPERTY(EditDefaultsOnly, Category = "Spells")
	TArray<TSubclassOf<AROWFireball>> SpellSlots;

	int32 SelectedSpellIndex = 0;

	FTimerHandle ManaRegenTimer;

	static constexpr float ManaRegenInterval = 0.1f;
};