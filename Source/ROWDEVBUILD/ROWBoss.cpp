// Copyright Holden Woods. Rite of Wizardry.

#include "ROWBoss.h"
#include "ROWFireball.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AROWBoss::AROWBoss()
{
	StartingHealth = 1000.f;
	CoinsToDrop = 10;
	KnockbackStrength = 0.f; // Bosses don't get shoved around.
}

void AROWBoss::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(BossAttackTimer, this, &AROWBoss::DoNextAttack, TimeBetweenAttacks, true, FirstAttackDelay);
}

void AROWBoss::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BossAttackTimer);
	Super::EndPlay(EndPlayReason);
}

void AROWBoss::DoNextAttack()
{
	if (bIsDead)
	{
		return;
	}

	const bool bCanSummon = MinionClass && GetMaxHealth() > 0.f && GetHealth() / GetMaxHealth() <= SummonHealthThreshold;

	// Pattern: burst, burst, summon (summon only once below the health threshold).
	if (bCanSummon && AttackStep % 3 == 2)
	{
		SummonMinions();
	}
	else
	{
		SpellBurst();
	}

	++AttackStep;
}

void AROWBoss::SpellBurst()
{
	if (!BurstProjectileClass)
	{
		return;
	}

	const bool bEnraged = GetMaxHealth() > 0.f && GetHealth() / GetMaxHealth() <= 0.5f;
	const int32 Count = BurstCount + (bEnraged ? EnragedExtraProjectiles : 0);

	// Rotate each ring a little so the safe gaps move.
	const float StartAngle = AttackStep * 15.f;

	FActorSpawnParameters Params;
	Params.Owner = this;      // So the bolts never hit the boss itself.
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 i = 0; i < Count; ++i)
	{
		const float Yaw = StartAngle + (360.f / Count) * i;
		const FRotator Rotation(0.f, Yaw, 0.f);
		const FVector Location = GetActorLocation() + Rotation.Vector() * BurstSpawnDistance;

		GetWorld()->SpawnActor<AROWFireball>(BurstProjectileClass, Location, Rotation, Params);
	}

	OnSpellBurst();
}

void AROWBoss::SummonMinions()
{
	Minions.RemoveAll([](const TWeakObjectPtr<AROWEnemy>& Minion) { return !Minion.IsValid(); });

	const int32 ToSpawn = FMath::Min(MinionsPerSummon, MaxMinions - Minions.Num());
	if (ToSpawn <= 0)
	{
		SpellBurst(); // Already at the cap: attack instead.
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	for (int32 i = 0; i < ToSpawn; ++i)
	{
		const float Yaw = FMath::FRandRange(0.f, 360.f);
		const FVector Location = GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * SummonDistance;

		if (AROWEnemy* Minion = GetWorld()->SpawnActor<AROWEnemy>(MinionClass, Location, FRotator::ZeroRotator, Params))
		{
			Minions.Add(Minion);
		}
	}

	OnSummon();
}

void AROWBoss::DropLoot()
{
	GetWorldTimerManager().ClearTimer(BossAttackTimer);

	// The fight is over: minions vanish with their master.
	for (const TWeakObjectPtr<AROWEnemy>& Minion : Minions)
	{
		if (Minion.IsValid())
		{
			Minion->Destroy();
		}
	}
	Minions.Reset();

	// Coins, like any enemy.
	Super::DropLoot();

	// Pick-1-of-N upgrades, laid out in a row at the boss's feet.
	if (!UpgradePickupClass || UpgradePool.Num() == 0)
	{
		return;
	}

	TArray<FROWUpgrade> Pool = UpgradePool;
	const int32 Count = FMath::Min(UpgradeChoices, Pool.Num());

	const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	const FVector Base = GetActorLocation() - FVector(0.f, 0.f, HalfHeight - 50.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<AROWUpgradePickup*> Spawned;
	TArray<FROWUpgrade> Chosen;
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Pick = FMath::RandRange(0, Pool.Num() - 1);
		Chosen.Add(Pool[Pick]);
		Pool.RemoveAtSwap(Pick);

		const float Offset = (i - (Count - 1) * 0.5f) * UpgradeSpacing;
		if (AROWUpgradePickup* Pickup = GetWorld()->SpawnActor<AROWUpgradePickup>(
			UpgradePickupClass, Base + FVector(0.f, Offset, 0.f), FRotator::ZeroRotator, Params))
		{
			Spawned.Add(Pickup);
		}
	}

	for (int32 i = 0; i < Spawned.Num(); ++i)
	{
		Spawned[i]->Setup(Chosen[i], Spawned);
	}
}