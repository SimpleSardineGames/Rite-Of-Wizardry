// Copyright Holden Woods. Rite of Wizardry.

#include "ROWFloorGenerator.h"
#include "ROWRoom.h"
#include "ROWEnemy.h"
#include "ROWCoinPickup.h"
#include "ROWExitPortal.h"
#include "ROWUpgradePickup.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	const FIntPoint GridDirections[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
	const uint8 GridDirectionSides[4] = { AROWRoom::Side_PosX, AROWRoom::Side_NegX, AROWRoom::Side_PosY, AROWRoom::Side_NegY };
}

AROWFloorGenerator::AROWFloorGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	SetRootComponent(Billboard);
}

void AROWFloorGenerator::BeginPlay()
{
	Super::BeginPlay();
	GenerateFloor();
}

int32 AROWFloorGenerator::CountNeighbours(const TSet<FIntPoint>& Cells, const FIntPoint& Cell) const
{
	int32 Count = 0;
	for (const FIntPoint& Dir : GridDirections)
	{
		if (Cells.Contains(Cell + Dir))
		{
			++Count;
		}
	}
	return Count;
}

bool AROWFloorGenerator::BuildLayout(FRandomStream& Stream, TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	TSet<FIntPoint> Occupied;

	OutCells.Add(FIntPoint(0, 0));
	Occupied.Add(FIntPoint(0, 0));

	// Grow outward from existing rooms. A new cell may only touch one existing room,
	// which keeps the floor branching like Isaac instead of filling in solid blocks.
	const int32 TargetRooms = GetRoomCountForFloor();
	for (int32 Attempt = 0; Attempt < 2000 && OutCells.Num() < TargetRooms; ++Attempt)
	{
		const FIntPoint From = OutCells[Stream.RandRange(0, OutCells.Num() - 1)];
		const FIntPoint Candidate = From + GridDirections[Stream.RandRange(0, 3)];

		if (Occupied.Contains(Candidate) || CountNeighbours(Occupied, Candidate) > 1)
		{
			continue;
		}

		OutCells.Add(Candidate);
		Occupied.Add(Candidate);
	}

	if (OutCells.Num() < TargetRooms)
	{
		return false;
	}

	// Need a dead end for the shop, plus one for the boss if a boss room is set.
	int32 DeadEnds = 0;
	for (int32 i = 1; i < OutCells.Num(); ++i)
	{
		if (CountNeighbours(Occupied, OutCells[i]) == 1)
		{
			++DeadEnds;
		}
	}

	const bool bShopOnFloor = ShopRoomClass && !bShopAfterBoss;
	const int32 Needed = (bShopOnFloor ? 1 : 0) + (BossRoomClass ? 1 : 0);
	return DeadEnds >= Needed;
}

void AROWFloorGenerator::GenerateFloor()
{
	if (!StartRoomClass || CombatRoomClasses.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ROWFloorGenerator: set Start Room Class and at least one Combat Room Class."));
		return;
	}

	// A fixed Seed still gives each floor its own layout.
	FRandomStream Stream(Seed != 0 ? Seed + FloorNumber * 7919 : FMath::Rand());

	// Retry until a layout has enough dead ends for the special rooms.
	TArray<FIntPoint> Cells;
	bool bLayoutOk = false;
	for (int32 Try = 0; Try < 50 && !bLayoutOk; ++Try)
	{
		bLayoutOk = BuildLayout(Stream, Cells);
	}

	if (!bLayoutOk)
	{
		UE_LOG(LogTemp, Warning, TEXT("ROWFloorGenerator: could not build a layout. Try a larger Room Count."));
		return;
	}

	TSet<FIntPoint> Occupied(Cells);

	// Distance from the start through the room graph (breadth-first), used to place the boss far away.
	TMap<FIntPoint, int32> Distance;
	TArray<FIntPoint> Queue;
	Distance.Add(Cells[0], 0);
	Queue.Add(Cells[0]);
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const FIntPoint Current = Queue[Head];
		for (const FIntPoint& Dir : GridDirections)
		{
			const FIntPoint Next = Current + Dir;
			if (Occupied.Contains(Next) && !Distance.Contains(Next))
			{
				Distance.Add(Next, Distance[Current] + 1);
				Queue.Add(Next);
			}
		}
	}

	// Collect dead ends (excluding the start room), farthest first.
	TArray<FIntPoint> DeadEnds;
	for (int32 i = 1; i < Cells.Num(); ++i)
	{
		if (CountNeighbours(Occupied, Cells[i]) == 1)
		{
			DeadEnds.Add(Cells[i]);
		}
	}
	DeadEnds.Sort([&Distance](const FIntPoint& A, const FIntPoint& B) { return Distance[A] > Distance[B]; });

	// Assign special rooms.
	TMap<FIntPoint, TSubclassOf<AROWRoom>> Assigned;
	Assigned.Add(Cells[0], StartRoomClass);

	FIntPoint BossCell(INT32_MAX, INT32_MAX);
	if (BossRoomClass && DeadEnds.Num() > 0)
	{
		BossCell = DeadEnds[0];
		Assigned.Add(BossCell, BossRoomClass);
		DeadEnds.RemoveAt(0);
	}

	if (ShopRoomClass && !bShopAfterBoss && DeadEnds.Num() > 0)
	{
		Assigned.Add(DeadEnds[Stream.RandRange(0, DeadEnds.Num() - 1)], ShopRoomClass);
	}

	// Spawn every room, telling each one which sides lead to a neighbour.
	const FVector Origin = GetActorLocation();
	for (const FIntPoint& Cell : Cells)
	{
		TSubclassOf<AROWRoom> RoomClass = Assigned.Contains(Cell)
			? Assigned[Cell]
			: CombatRoomClasses[Stream.RandRange(0, CombatRoomClasses.Num() - 1)];

		if (!RoomClass)
		{
			continue;
		}

		uint8 OpenSides = 0;
		for (int32 d = 0; d < 4; ++d)
		{
			if (Occupied.Contains(Cell + GridDirections[d]))
			{
				OpenSides |= GridDirectionSides[d];
			}
		}

		const FTransform SpawnTransform(FRotator::ZeroRotator, Origin + FVector(Cell.X * CellSize, Cell.Y * CellSize, 0.f));

		// Deferred spawn so the open sides are set before the room's BeginPlay runs.
		if (AROWRoom* Room = GetWorld()->SpawnActorDeferred<AROWRoom>(RoomClass, SpawnTransform))
		{
			Room->SetOpenSides(OpenSides);
			Room->SetBonusEnemies((FloorNumber - 1) * ExtraEnemiesPerFloor);
			Room->FinishSpawning(SpawnTransform);
			SpawnedRooms.Add(Room);

			// Beating the boss ends the floor.
			if (Cell == BossCell && bShopAfterBoss)
			{
				Room->OnCleared.AddDynamic(this, &AROWFloorGenerator::HandleBossRoomCleared);
			}
		}
	}

	// The post-boss shop: a closed-off room well away from the floor, reached only by teleport.
	if (bShopAfterBoss && ShopRoomClass)
	{
		if (!BossRoomClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("ROWFloorGenerator: Shop After Boss is on but no Boss Room Class is set, so the shop can't be reached."));
		}

		const FTransform ShopTransform(FRotator::ZeroRotator, Origin + FVector(-(GetRoomCountForFloor() + 2) * CellSize, 0.f, 0.f));
		if (AROWRoom* Room = GetWorld()->SpawnActorDeferred<AROWRoom>(ShopRoomClass, ShopTransform))
		{
			Room->SetOpenSides(0); // Every door is a wall.
			Room->FinishSpawning(ShopTransform);
			ShopRoom = Room;
		}
	}

	// Put the player in the start room.
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		PlayerPawn->SetActorLocation(Origin + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);
	}

	OnFloorStarted(FloorNumber);
}

void AROWFloorGenerator::StartNextFloor()
{
	// Wait a frame: this is usually called from the portal's own overlap event,
	// and the portal gets destroyed as part of the rebuild.
	GetWorldTimerManager().SetTimerForNextTick(this, &AROWFloorGenerator::DoStartNextFloor);
}

void AROWFloorGenerator::DoStartNextFloor()
{
	ClearFloor();
	++FloorNumber;
	GenerateFloor();
}

void AROWFloorGenerator::ClearFloor()
{
	GetWorldTimerManager().ClearTimer(ShopTeleportTimer);

	for (AROWRoom* Room : SpawnedRooms)
	{
		if (Room)
		{
			Room->Destroy();
		}
	}
	SpawnedRooms.Reset();

	if (ShopRoom)
	{
		ShopRoom->Destroy();
		ShopRoom = nullptr;
	}

	if (ExitPortal)
	{
		ExitPortal->Destroy();
		ExitPortal = nullptr;
	}

	if (ShopPortal)
	{
		ShopPortal->Destroy();
		ShopPortal = nullptr;
	}
	BossRoom = nullptr;

	// Any boss upgrades the player walked past.
	TArray<AActor*> Upgrades;
	UGameplayStatics::GetAllActorsOfClass(this, AROWUpgradePickup::StaticClass(), Upgrades);
	for (AActor* Actor : Upgrades)
	{
		Actor->Destroy();
	}

	// Leftover coins and any stray enemies from the old floor.
	TArray<AActor*> Leftovers;
	UGameplayStatics::GetAllActorsOfClass(this, AROWCoinPickup::StaticClass(), Leftovers);
	for (AActor* Actor : Leftovers)
	{
		Actor->Destroy();
	}

	UGameplayStatics::GetAllActorsOfClass(this, AROWEnemy::StaticClass(), Leftovers);
	for (AActor* Actor : Leftovers)
	{
		Actor->Destroy();
	}
}

void AROWFloorGenerator::HandleBossRoomCleared(AROWRoom* Room)
{
	OnFloorCompleted();
	BossRoom = Room;

	if (!ShopRoom)
	{
		return;
	}

	if (ExitPortalClass)
	{
		// The player walks into a portal when ready, after picking up the boss's upgrade.
		GetWorldTimerManager().SetTimer(ShopTeleportTimer, this, &AROWFloorGenerator::SpawnShopPortal,
			FMath::Max(ShopTeleportDelay, 0.01f), false);
	}
	else
	{
		// No portal set up: fall back to an automatic teleport.
		GetWorldTimerManager().SetTimer(ShopTeleportTimer, this, &AROWFloorGenerator::SendPlayerToShop,
			FMath::Max(ShopTeleportDelay, 0.01f), false);
	}
}

void AROWFloorGenerator::SpawnShopPortal()
{
	if (!BossRoom || !ExitPortalClass || ShopPortal)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	ShopPortal = GetWorld()->SpawnActor<AActor>(ExitPortalClass, BossRoom->GetActorLocation() + ExitPortalOffset,
		FRotator::ZeroRotator, Params);

	if (AROWExitPortal* Portal = Cast<AROWExitPortal>(ShopPortal))
	{
		Portal->SetLeadsToShop(true);
	}
}

void AROWFloorGenerator::TravelToShop()
{
	// Wait a frame: called from the portal's own overlap event, and that portal is removed on arrival.
	GetWorldTimerManager().SetTimerForNextTick(this, &AROWFloorGenerator::SendPlayerToShop);
}

void AROWFloorGenerator::SendPlayerToShop()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn || !ShopRoom)
	{
		return;
	}

	if (ShopPortal)
	{
		ShopPortal->Destroy();
		ShopPortal = nullptr;
	}

	PlayerPawn->SetActorLocation(ShopRoom->GetActorLocation() + FVector(0.f, 0.f, 150.f), false, nullptr, ETeleportType::TeleportPhysics);

	// Open the way to the next floor.
	if (ExitPortalClass && !ExitPortal)
	{
		FActorSpawnParameters Params;
		Params.Owner = this; // The portal calls back to this generator.
		ExitPortal = GetWorld()->SpawnActor<AActor>(ExitPortalClass, ShopRoom->GetActorLocation() + ExitPortalOffset,
			FRotator::ZeroRotator, Params);
	}

	OnPlayerEnteredShop();
}