// Copyright Holden Woods. Rite of Wizardry.

#include "ROWRangedEnemy.h"
#include "ROWFireball.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

AROWRangedEnemy::AROWRangedEnemy()
{
	StartingHealth = 60.f;   // Fragile: dangerous at range, weak up close.
	KnockbackStrength = 400.f;
}

void AROWRangedEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(ThinkTimer, this, &AROWRangedEnemy::Think, 0.3f, true, 0.2f);

	// Random first shot so a group of casters doesn't fire in perfect sync.
	GetWorldTimerManager().SetTimer(FireTimer, this, &AROWRangedEnemy::FireAtPlayer, FireInterval, true,
		FMath::FRandRange(0.5f, FireInterval));
}

void AROWRangedEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	GetWorldTimerManager().ClearTimer(FireTimer);
	Super::EndPlay(EndPlayReason);
}

void AROWRangedEnemy::Think()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	AAIController* AI = Cast<AAIController>(GetController());
	if (bIsDead || !Player || !AI)
	{
		return;
	}

	const float Distance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());

	if (Distance > MaxRange)
	{
		// Too far: close in until just inside firing range.
		AI->MoveToActor(Player, MaxRange * 0.8f);
	}
	else if (Distance < MinRange)
	{
		// Too close: back away to a walkable spot behind us.
		const FVector Away = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
		FVector Target = GetActorLocation() + Away * 400.f;

		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(GetWorld()))
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(Target, NavLocation, FVector(200.f, 200.f, 200.f)))
			{
				Target = NavLocation.Location;
			}
		}

		AI->MoveToLocation(Target, 50.f);
	}
	else
	{
		// In the sweet spot: stop and face the player.
		AI->StopMovement();
		const FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		SetActorRotation(ToPlayer.Rotation());
	}
}

void AROWRangedEnemy::FireAtPlayer()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	AAIController* AI = Cast<AAIController>(GetController());
	if (bIsDead || !Player || !ProjectileClass)
	{
		return;
	}

	// Only shoot when in range and with a clear line of sight (not through walls or pillars).
	const float Distance = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	if (Distance > MaxRange || (AI && !AI->LineOfSightTo(Player)))
	{
		return;
	}

	const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<AROWFireball>(ProjectileClass, GetActorLocation() + Direction * ProjectileSpawnDistance,
		Direction.Rotation(), Params);

	OnFired();
}