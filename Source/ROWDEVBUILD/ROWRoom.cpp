// Copyright Holden Woods. Rite of Wizardry.

#include "ROWRoom.h"
#include "ROWEnemy.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

AROWRoom::AROWRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	RoomBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBounds"));
	RoomBounds->SetBoxExtent(FVector(1000.f, 1000.f, 200.f));
	RoomBounds->SetCollisionProfileName(TEXT("Trigger"));
	RoomBounds->SetGenerateOverlapEvents(true);
	RoomBounds->ShapeColor = FColor::Green;
	SetRootComponent(RoomBounds);
}

void AROWRoom::BeginPlay()
{
	Super::BeginPlay();

	// Collect door components built into this room's Blueprint (tagged "Door").
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive && Primitive->ComponentHasTag(DoorComponentTag))
		{
			uint8 Side = 0;
			if (Primitive->ComponentHasTag(TEXT("PosX"))) { Side = Side_PosX; }
			else if (Primitive->ComponentHasTag(TEXT("NegX"))) { Side = Side_NegX; }
			else if (Primitive->ComponentHasTag(TEXT("PosY"))) { Side = Side_PosY; }
			else if (Primitive->ComponentHasTag(TEXT("NegY"))) { Side = Side_NegY; }

			DoorComponents.Add(Primitive);
			DoorComponentSides.Add(Side);
		}
	}

	// Doors start open so the player can walk in.
	SetDoorsLocked(false);

	RoomBounds->OnComponentBeginOverlap.AddDynamic(this, &AROWRoom::OnBoundsBeginOverlap);
}

void AROWRoom::OnBoundsBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (RoomState != EROWRoomState::Waiting)
	{
		return;
	}

	// Only the player starts a room, not enemies or spells.
	if (OtherActor && OtherActor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		StartRoom();
	}
}

void AROWRoom::StartRoom()
{
	// Safe room: nothing to fight, so it counts as cleared right away.
	if (EnemyCount <= 0 || EnemyTypes.Num() == 0)
	{
		ClearRoom();
		return;
	}

	RoomState = EROWRoomState::Active;
	SetDoorsLocked(true);
	OnRoomStarted();

	GetWorldTimerManager().SetTimer(SpawnTimer, this, &AROWRoom::SpawnEnemies, FMath::Max(SpawnDelay, 0.01f), false);
}

void AROWRoom::SpawnEnemies()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < EnemyCount + BonusEnemies; ++i)
	{
		const TSubclassOf<AROWEnemy> EnemyClass = EnemyTypes[FMath::RandRange(0, EnemyTypes.Num() - 1)];
		if (!EnemyClass)
		{
			continue;
		}

		FVector SpawnLocation;
		if (!FindSpawnPoint(SpawnLocation))
		{
			continue;
		}

		if (AROWEnemy* Enemy = GetWorld()->SpawnActor<AROWEnemy>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Enemy->OnDestroyed.AddDynamic(this, &AROWRoom::HandleEnemyDestroyed);
			++AliveEnemies;
		}
	}

	// If nothing could spawn (no nav mesh, empty types), don't trap the player.
	if (AliveEnemies == 0)
	{
		ClearRoom();
	}
}

bool AROWRoom::FindSpawnPoint(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	const FBox Box = RoomBounds->Bounds.GetBox();
	const float FloorZ = GetActorLocation().Z;

	// Find the walkable floor near the room's centre. Points are then picked only where
	// an enemy could walk from here, which rules out wall and pillar tops (separate nav islands).
	FNavLocation FloorOrigin;
	const bool bHaveFloorOrigin = NavSys && NavSys->ProjectPointToNavigation(
		FVector(Box.GetCenter().X, Box.GetCenter().Y, FloorZ), FloorOrigin, FVector(600.f, 600.f, 100.f));

	const float SearchRadius = FMath::Min(Box.GetExtent().X, Box.GetExtent().Y);

	// Try a handful of random points; keep the first walkable one far enough from the player.
	for (int32 Attempt = 0; Attempt < 25; ++Attempt)
	{
		FVector Candidate;

		if (bHaveFloorOrigin)
		{
			FNavLocation NavLocation;
			if (!NavSys->GetRandomReachablePointInRadius(FloorOrigin.Location, SearchRadius, NavLocation))
			{
				continue;
			}
			Candidate = NavLocation.Location;
		}
		else
		{
			// No nav mesh found near the centre: fall back to any point in the bounds at floor height.
			Candidate = FMath::RandPointInBox(Box);
			Candidate.Z = FloorZ;
		}

		// Stay inside this room (reachable points can leak through open doors into neighbours).
		if (Candidate.X < Box.Min.X || Candidate.X > Box.Max.X || Candidate.Y < Box.Min.Y || Candidate.Y > Box.Max.Y)
		{
			continue;
		}

		// Never on top of walls, pillars or other raised objects.
		if (Candidate.Z > FloorZ + MaxSpawnHeightAboveFloor)
		{
			continue;
		}

		if (PlayerPawn && FVector::Dist2D(Candidate, PlayerPawn->GetActorLocation()) < MinDistanceFromPlayer)
		{
			continue;
		}

		// Lift so the capsule doesn't start inside the floor.
		OutLocation = Candidate + FVector(0.f, 0.f, 100.f);
		return true;
	}

	return false;
}

void AROWRoom::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	AliveEnemies = FMath::Max(AliveEnemies - 1, 0);

	if (AliveEnemies == 0 && RoomState == EROWRoomState::Active)
	{
		ClearRoom();
	}
}

void AROWRoom::ClearRoom()
{
	RoomState = EROWRoomState::Cleared;
	SetDoorsLocked(false);
	OnRoomCleared();
	OnCleared.Broadcast(this);
}

void AROWRoom::SetDoorsLocked(bool bLocked)
{
	for (AActor* Door : Doors)
	{
		if (Door)
		{
			Door->SetActorHiddenInGame(!bLocked);
			Door->SetActorEnableCollision(bLocked);
		}
	}

	for (int32 i = 0; i < DoorComponents.Num(); ++i)
	{
		UPrimitiveComponent* DoorComponent = DoorComponents[i];
		if (!DoorComponent)
		{
			continue;
		}

		// A door on a side with no neighbouring room is a permanent wall.
		const uint8 Side = DoorComponentSides.IsValidIndex(i) ? DoorComponentSides[i] : 0;
		const bool bSideClosed = Side != 0 && (OpenSides & Side) == 0;
		const bool bBlocking = bLocked || bSideClosed;

		DoorComponent->SetVisibility(bBlocking, true);
		DoorComponent->SetCollisionEnabled(bBlocking ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}