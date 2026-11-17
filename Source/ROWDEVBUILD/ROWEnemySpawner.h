// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWEnemySpawner.generated.h"

class AROWEnemy;
class UBillboardComponent;

/**
 * Place in a level to spawn enemies on a timer at random reachable points around it.
 * Keeps no more than MaxAlive of its own enemies alive at once.
 */
UCLASS()
class ROWDEVBUILD_API AROWEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AROWEnemySpawner();

protected:
	virtual void BeginPlay() override;

	void SpawnEnemy();

	// Enemy Blueprint to spawn (for example BP_Enemy).
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AROWEnemy> EnemyClass;

	// Seconds between spawn attempts.
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = "0.1"))
	float SpawnInterval = 3.f;

	// Enemies spawn at a random reachable point within this distance of the spawner.
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = "0"))
	float SpawnRadius = 800.f;

	// Maximum number of this spawner's enemies alive at the same time.
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = "1"))
	int32 MaxAlive = 5;

	// Seconds before the first spawn, giving the player a moment at the start.
	UPROPERTY(EditAnywhere, Category = "Spawning", meta = (ClampMin = "0"))
	float InitialDelay = 2.f;

	// Editor-only icon so the spawner is easy to find in the level.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBillboardComponent> Billboard;

	TArray<TWeakObjectPtr<AROWEnemy>> AliveEnemies;

	FTimerHandle SpawnTimer;
};