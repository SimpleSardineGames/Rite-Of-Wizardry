// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "ROWEnemy.h"
#include "ROWUpgradePickup.h"
#include "ROWBoss.generated.h"

class AROWFireball;

/**
 * The floor boss. Cycles through attacks on a timer:
 *  - Spell burst: a ring of projectiles in every direction (dodge through the gaps).
 *  - Summon: calls in minions, but only once its health drops below SummonHealthThreshold.
 * On death its minions vanish and it drops UpgradeChoices upgrades; the player takes one.
 * Chasing (if any) is set up in the Blueprint like BP_Enemy.
 */
UCLASS()
class ROWDEVBUILD_API AROWBoss : public AROWEnemy
{
	GENERATED_BODY()

public:
	AROWBoss();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DropLoot() override;

	void DoNextAttack();
	void SpellBurst();
	void SummonMinions();

	// Blueprint hooks for wind-up animations, sounds or warnings.
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void OnSpellBurst();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void OnSummon();

	// ---- Spell burst ----

	// Projectile fired in the burst (for example BP_BossBolt, a child of ROWFireball).
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Burst")
	TSubclassOf<AROWFireball> BurstProjectileClass;

	// Projectiles per ring.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Burst", meta = (ClampMin = "1"))
	int32 BurstCount = 12;

	// Extra projectiles per ring once the boss is below half health (it gets angrier).
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Burst", meta = (ClampMin = "0"))
	int32 EnragedExtraProjectiles = 6;

	// How far from the boss's centre the projectiles start.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Burst", meta = (ClampMin = "0"))
	float BurstSpawnDistance = 250.f;

	// ---- Summon ----

	// Minion to summon (for example BP_Enemy).
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon")
	TSubclassOf<AROWEnemy> MinionClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (ClampMin = "0"))
	int32 MinionsPerSummon = 2;

	// Never more than this many minions alive at once.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (ClampMin = "0"))
	int32 MaxMinions = 4;

	// Summoning starts once health falls below this fraction (0.6 = 60%).
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (ClampMin = "0", ClampMax = "1"))
	float SummonHealthThreshold = 0.6f;

	// Minions appear this far from the boss.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Summon", meta = (ClampMin = "0"))
	float SummonDistance = 350.f;

	// ---- Timing ----

	// Seconds between attacks.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Timing", meta = (ClampMin = "0.1"))
	float TimeBetweenAttacks = 2.5f;

	// Seconds before the first attack, so the player can get their bearings.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Timing", meta = (ClampMin = "0"))
	float FirstAttackDelay = 1.5f;

	// ---- Reward ----

	// Every upgrade the boss could drop. UpgradeChoices of them are offered.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reward")
	TArray<FROWUpgrade> UpgradePool;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reward", meta = (ClampMin = "1"))
	int32 UpgradeChoices = 3;

	// Pickup actor for each choice (for example BP_UpgradePickup).
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reward")
	TSubclassOf<AROWUpgradePickup> UpgradePickupClass;

	// Gap between the choices.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Reward", meta = (ClampMin = "0"))
	float UpgradeSpacing = 300.f;

	TArray<TWeakObjectPtr<AROWEnemy>> Minions;

	FTimerHandle BossAttackTimer;

	int32 AttackStep = 0;
};