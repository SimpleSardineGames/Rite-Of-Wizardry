// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "ROWEnemy.h"
#include "ROWRangedEnemy.generated.h"

class AROWFireball;

/**
 * An enemy caster that keeps its distance and shoots. Movement is handled here in C++,
 * so its Blueprint must NOT have the melee chase timer from BP_Enemy:
 *  - farther than MaxRange: walks toward the player
 *  - closer than MinRange: backs away
 *  - in between: stands still, faces the player, and fires every FireInterval (if it can see them)
 */
UCLASS()
class ROWDEVBUILD_API AROWRangedEnemy : public AROWEnemy
{
	GENERATED_BODY()

public:
	AROWRangedEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Think();
	void FireAtPlayer();

	// Blueprint hook for a casting animation or sound.
	UFUNCTION(BlueprintImplementableEvent, Category = "Ranged")
	void OnFired();

	// Projectile to shoot (for example BP_EnemyBolt, a child of ROWFireball).
	UPROPERTY(EditDefaultsOnly, Category = "Ranged")
	TSubclassOf<AROWFireball> ProjectileClass;

	// Backs away if the player gets closer than this.
	UPROPERTY(EditDefaultsOnly, Category = "Ranged", meta = (ClampMin = "0"))
	float MinRange = 500.f;

	// Walks closer if the player is farther than this. Also the maximum firing range.
	UPROPERTY(EditDefaultsOnly, Category = "Ranged", meta = (ClampMin = "0"))
	float MaxRange = 1100.f;

	// Seconds between shots.
	UPROPERTY(EditDefaultsOnly, Category = "Ranged", meta = (ClampMin = "0.1"))
	float FireInterval = 1.8f;

	// Projectiles start this far in front of the enemy.
	UPROPERTY(EditDefaultsOnly, Category = "Ranged", meta = (ClampMin = "0"))
	float ProjectileSpawnDistance = 100.f;

	FTimerHandle ThinkTimer;
	FTimerHandle FireTimer;
};