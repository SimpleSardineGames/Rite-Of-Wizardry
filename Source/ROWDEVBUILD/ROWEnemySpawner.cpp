// Copyright Holden Woods. Rite of Wizardry.

#include "ROWEnemySpawner.h"
#include "ROWEnemy.h"
#include "Components/BillboardComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

AROWEnemySpawner::AROWEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(Billboard);
}

void AROWEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AROWEnemySpawner::SpawnEnemy, SpawnInterval, true, InitialDelay);
}

void AROWEnemySpawner::SpawnEnemy()
{
	if (!EnemyClass)
	{
		return;
	}

	// Forget enemies that have been destroyed.
	AliveEnemies.RemoveAll([](const TWeakObjectPtr<AROWEnemy>& Enemy) { return !Enemy.IsValid(); });

	if (AliveEnemies.Num() >= MaxAlive)
	{
		return;
	}

	// Pick a random point the enemy can actually walk to; fall back to the spawner's own location.
	FVector SpawnLocation = GetActorLocation();
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(GetWorld()))
	{
		FNavLocation NavLocation;
		if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation))
		{
			SpawnLocation = NavLocation.Location;
		}
	}

	// Lift the spawn point so the capsule doesn't start inside the floor.
	SpawnLocation.Z += 100.f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	if (AROWEnemy* Enemy = GetWorld()->SpawnActor<AROWEnemy>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, Params))
	{
		AliveEnemies.Add(Enemy);
	}
}