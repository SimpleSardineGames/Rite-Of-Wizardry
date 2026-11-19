// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWCoinPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * A coin dropped by enemies. The player collects it by walking over it.
 */
UCLASS()
class ROWDEVBUILD_API AROWCoinPickup : public AActor
{
	GENERATED_BODY()

public:
	AROWCoinPickup();

protected:
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Blueprint hook for a pickup sound or sparkle.
	UFUNCTION(BlueprintImplementableEvent, Category = "Coin")
	void OnCollected();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupSphere;

	// Visual only; assign a coin mesh in the Blueprint child.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// How many coins this pickup is worth.
	UPROPERTY(EditDefaultsOnly, Category = "Coin", meta = (ClampMin = "1"))
	int32 Value = 1;
};