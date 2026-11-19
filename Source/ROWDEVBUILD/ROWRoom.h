// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWRoom.generated.h"

class AROWEnemy;
class UBoxComponent;

class AROWRoom;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomClearedSignature, AROWRoom*, Room);

UENUM(BlueprintType)
enum class EROWRoomState : uint8
{
	Waiting,	// Player hasn't entered yet.
	Active,		// Doors locked, enemies alive.
	Cleared		// All enemies dead, doors open for good.
};

/**
 * An Isaac-style combat room. Scale RoomBounds to cover the room's floor.
 * When the player walks in: doors lock, enemies spawn inside the bounds.
 * When every enemy is dead: doors unlock and the room stays cleared.
 * Doors are any actors placed in the level (for example BP_Door): locked = visible and solid,
 * unlocked = hidden with no collision.
 */
UCLASS()
class ROWDEVBUILD_API AROWRoom : public AActor
{
	GENERATED_BODY()

public:
	AROWRoom();

	UFUNCTION(BlueprintPure, Category = "Room")
	EROWRoomState GetRoomState() const { return RoomState; }

	// Fired once when the room is cleared. The floor generator listens to the boss room's.
	UPROPERTY(BlueprintAssignable, Category = "Room")
	FOnRoomClearedSignature OnCleared;

	// Side flags for SetOpenSides. Combine with | (for example PosX | NegY).
	static constexpr uint8 Side_PosX = 1;
	static constexpr uint8 Side_NegX = 2;
	static constexpr uint8 Side_PosY = 4;
	static constexpr uint8 Side_NegY = 8;
	static constexpr uint8 Side_All = 15;

	// Which sides lead to a neighbouring room. Door components on any other side stay shut
	// for good, acting as walls. Called by the floor generator before the room finishes spawning.
	void SetOpenSides(uint8 InOpenSides) { OpenSides = InOpenSides; }

	// Extra enemies on top of EnemyCount (later floors are harder). Safe rooms stay safe.
	void SetBonusEnemies(int32 InBonusEnemies) { BonusEnemies = bIgnoreFloorBonus ? 0 : FMath::Max(InBonusEnemies, 0); }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBoundsBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);

	void StartRoom();
	void SpawnEnemies();
	void ClearRoom();
	void SetDoorsLocked(bool bLocked);

	// Finds a walkable spot inside the room, away from the player. Returns false if none found.
	bool FindSpawnPoint(FVector& OutLocation) const;

	// Blueprint hooks for sounds, music, rewards, and so on.
	UFUNCTION(BlueprintImplementableEvent, Category = "Room")
	void OnRoomStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Room")
	void OnRoomCleared();

	// Covers the room's floor. Entering it starts the room.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> RoomBounds;

	// Door actors placed in the level that block this room's exits while it's active.
	UPROPERTY(EditInstanceOnly, Category = "Room")
	TArray<TObjectPtr<AActor>> Doors;

	// Components inside this room's own Blueprint that act as doors.
	// Any component given the tag in DoorComponentTag is collected automatically at BeginPlay.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> DoorComponents;

	// Which side each entry in DoorComponents faces (0 = no side tag, always treated as open).
	TArray<uint8> DoorComponentSides;

	// Tag that marks a component in the room Blueprint as a door.
	// Also give each door a side tag: PosX, NegX, PosY or NegY (which way it faces from the room centre).
	UPROPERTY(EditDefaultsOnly, Category = "Room")
	FName DoorComponentTag = TEXT("Door");

	// Sides that connect to another room. Hand-placed rooms keep all sides open.
	uint8 OpenSides = Side_All;

	// Enemy types to pick from at random (for example BP_Enemy).
	UPROPERTY(EditAnywhere, Category = "Room")
	TArray<TSubclassOf<AROWEnemy>> EnemyTypes;

	// How many enemies spawn. 0 = a safe room (no lock, no enemies).
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "0"))
	int32 EnemyCount = 4;

	// Tick for the boss room, so later floors don't add extra bosses.
	UPROPERTY(EditAnywhere, Category = "Room")
	bool bIgnoreFloorBonus = false;

	// Seconds between the doors locking and enemies appearing.
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "0"))
	float SpawnDelay = 0.75f;

	// Enemies won't spawn closer than this to the player.
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "0"))
	float MinDistanceFromPlayer = 400.f;

	// Spawn points higher than this above the room's origin (floor level) are rejected,
	// so enemies never appear on walls or pillars.
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "0"))
	float MaxSpawnHeightAboveFloor = 50.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Room")
	EROWRoomState RoomState = EROWRoomState::Waiting;

	int32 AliveEnemies = 0;

	int32 BonusEnemies = 0;

	FTimerHandle SpawnTimer;
};