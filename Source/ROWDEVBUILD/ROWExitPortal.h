// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWExitPortal.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * Portal in the shop that leads to the next floor. The floor generator spawns it with itself as Owner;
 * walking into it tells the generator to build the next floor.
 */
UCLASS()
class ROWDEVBUILD_API AROWExitPortal : public AActor
{
	GENERATED_BODY()

public:
	AROWExitPortal();

	// True = this portal is in the boss room and leads to the shop. False = in the shop, leads to the next floor.
	void SetLeadsToShop(bool bInLeadsToShop) { bLeadsToShop = bInLeadsToShop; }

protected:
	UFUNCTION()
	void OnPortalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Blueprint hook for a whoosh sound or flash.
	UFUNCTION(BlueprintImplementableEvent, Category = "Portal")
	void OnPortalUsed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Trigger;

	// Visual only; assign a mesh and glowing material in the Blueprint.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	bool bUsed = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Portal")
	bool bLeadsToShop = false;
};