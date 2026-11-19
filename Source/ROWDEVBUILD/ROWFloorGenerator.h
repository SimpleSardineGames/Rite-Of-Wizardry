// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWFloorGenerator.generated.h"

class AROWRoom;
class UBillboardComponent;

/**
 * Builds a random Isaac-style floor at BeginPlay: rooms on a square grid, branching out from a start room.
 * The shop goes in a dead end; if a boss room class is set, it goes in the dead end farthest from the start.
 * Place one in the level where the start room should be. The player is moved into the start room.
 */
UCLASS()
class ROWDEVBUILD_API AROWFloorGenerator : public AActor
{
	GENERATED_BODY()

public:
	AROWFloorGenerator();

	// Builds the floor. Called automatically at BeginPlay.
	UFUNCTION(BlueprintCallable, Category = "Floor")
	void GenerateFloor();

	// Removes the current floor and builds the next, harder one. Called by the exit portal.
	UFUNCTION(BlueprintCallable, Category = "Floor")
	void StartNextFloor();

	// 1 on the first floor, 2 on the next, and so on.
	UFUNCTION(BlueprintPure, Category = "Floor")
	int32 GetFloorNumber() const { return FloorNumber; }

	// Moves the player into the shop. Called by the portal that appears after the boss dies.
	UFUNCTION(BlueprintCallable, Category = "Floor")
	void TravelToShop();

protected:
	virtual void BeginPlay() override;

	// Lays out grid cells. Returns false if the layout lacks enough dead ends for special rooms.
	bool BuildLayout(FRandomStream& Stream, TArray<FIntPoint>& OutCells) const;

	int32 CountNeighbours(const TSet<FIntPoint>& Cells, const FIntPoint& Cell) const;

	UFUNCTION()
	void HandleBossRoomCleared(AROWRoom* Room);

	void SendPlayerToShop();

	// Opens the portal to the shop in the cleared boss room.
	void SpawnShopPortal();

	UPROPERTY(Transient)
	TObjectPtr<AROWRoom> BossRoom;

	// Portal in the boss room that leads to the shop.
	UPROPERTY(Transient)
	TObjectPtr<AActor> ShopPortal;

	// Destroys every room, the portal, and leftover coins and enemies.
	void ClearFloor();

	void DoStartNextFloor();

	// Blueprint hook: a new floor has just been built (good place for "Floor 2" text).
	UFUNCTION(BlueprintImplementableEvent, Category = "Floor")
	void OnFloorStarted(int32 NewFloorNumber);

	// Portal placed in the shop that leads to the next floor (for example BP_ExitPortal).
	UPROPERTY(EditAnywhere, Category = "Floor|Progression")
	TSubclassOf<AActor> ExitPortalClass;

	// Where the portal appears, relative to the shop room's centre.
	UPROPERTY(EditAnywhere, Category = "Floor|Progression")
	FVector ExitPortalOffset = FVector(1200.f, 0.f, 100.f);

	// Extra enemies added to every combat room for each floor after the first.
	UPROPERTY(EditAnywhere, Category = "Floor|Progression", meta = (ClampMin = "0"))
	int32 ExtraEnemiesPerFloor = 1;

	// Extra rooms added to the floor for each floor after the first.
	UPROPERTY(EditAnywhere, Category = "Floor|Progression", meta = (ClampMin = "0"))
	int32 ExtraRoomsPerFloor = 1;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ExitPortal;

	int32 FloorNumber = 1;

	// Room count for the current floor (RoomCount plus the per-floor growth).
	int32 GetRoomCountForFloor() const { return FMath::Clamp(RoomCount + (FloorNumber - 1) * ExtraRoomsPerFloor, 3, 40); }

	// Blueprint hook: the floor is beaten (good place for a fade-out, sound or "Floor Complete" text).
	UFUNCTION(BlueprintImplementableEvent, Category = "Floor")
	void OnFloorCompleted();

	// Blueprint hook: the player has arrived in the shop.
	UFUNCTION(BlueprintImplementableEvent, Category = "Floor")
	void OnPlayerEnteredShop();

	// True = the shop is a separate room the player is sent to after beating the boss.
	// False = the shop is placed in a dead end on the floor like any other room.
	UPROPERTY(EditAnywhere, Category = "Floor|Rooms")
	bool bShopAfterBoss = true;

	// Seconds between the boss dying and the portal to the shop opening (time to pick an upgrade first).
	UPROPERTY(EditAnywhere, Category = "Floor|Rooms", meta = (ClampMin = "0", EditCondition = "bShopAfterBoss"))
	float ShopTeleportDelay = 2.f;

	UPROPERTY(Transient)
	TObjectPtr<AROWRoom> ShopRoom;

	FTimerHandle ShopTeleportTimer;

	// Room the player starts in (usually a safe room with Enemy Count 0).
	UPROPERTY(EditAnywhere, Category = "Floor|Rooms")
	TSubclassOf<AROWRoom> StartRoomClass;

	// Combat rooms picked at random for every ordinary cell.
	UPROPERTY(EditAnywhere, Category = "Floor|Rooms")
	TArray<TSubclassOf<AROWRoom>> CombatRoomClasses;

	// The shop. Placed in a random dead end.
	UPROPERTY(EditAnywhere, Category = "Floor|Rooms")
	TSubclassOf<AROWRoom> ShopRoomClass;

	// Optional. Placed in the dead end farthest from the start. Leave empty until the boss exists.
	UPROPERTY(EditAnywhere, Category = "Floor|Rooms")
	TSubclassOf<AROWRoom> BossRoomClass;

	// Total rooms on the floor, including start, shop and boss.
	UPROPERTY(EditAnywhere, Category = "Floor", meta = (ClampMin = "3", ClampMax = "40"))
	int32 RoomCount = 8;

	// Distance between room centres. Must match your room size (for example 2000 for 2000x2000 rooms).
	UPROPERTY(EditAnywhere, Category = "Floor", meta = (ClampMin = "100"))
	float CellSize = 2000.f;

	// 0 = a different floor every run. Any other number always builds the same floor (handy for testing).
	UPROPERTY(EditAnywhere, Category = "Floor")
	int32 Seed = 0;

	// Editor-only icon so the generator is easy to find in the level.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AROWRoom>> SpawnedRooms;
};