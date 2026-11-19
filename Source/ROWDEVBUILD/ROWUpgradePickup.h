// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWUpgradePickup.generated.h"

class UGameplayEffect;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UTexture2D;

USTRUCT(BlueprintType)
struct FROWUpgrade
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	TObjectPtr<UTexture2D> Icon;

	// Permanent effect applied when taken (for example GE_Upgrade_SpellPower).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	TSubclassOf<UGameplayEffect> Effect;
};

/**
 * An upgrade dropped by the boss. Bosses drop several; taking one removes the others (pick 1 of 3).
 */
UCLASS()
class ROWDEVBUILD_API AROWUpgradePickup : public AActor
{
	GENERATED_BODY()

public:
	AROWUpgradePickup();

	// Called by the boss right after spawning.
	void Setup(const FROWUpgrade& InUpgrade, const TArray<AROWUpgradePickup*>& InGroup);

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	const FROWUpgrade& GetUpgrade() const { return Upgrade; }

protected:
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Blueprint hooks: show the icon on a widget, play a sound, and so on.
	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void OnSetup(const FROWUpgrade& InUpgrade);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void OnTaken();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupSphere;

	// Visual only; assign a pedestal or orb mesh in the Blueprint.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Floating name above the pickup.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> NameText;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Upgrade")
	FROWUpgrade Upgrade;

	// The other choices dropped alongside this one.
	TArray<TWeakObjectPtr<AROWUpgradePickup>> Group;

	bool bTaken = false;
};