// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWFireball.generated.h"

class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * Fireball projectile. Flies straight along its forward vector and, on hitting an actor
 * that has an Ability System Component, applies its damage Gameplay Effect to it.
 * Spawn it with the shooter set as Owner so it ignores the caster.
 */
UCLASS()
class ROWDEVBUILD_API AROWFireball : public AActor
{
	GENERATED_BODY()

public:
	AROWFireball();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Sphere;

	// Visual only; assign a mesh in the Blueprint child.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// Gameplay Effect applied to whatever the fireball hits (for example GE_Damage_Test).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball")
	TSubclassOf<UGameplayEffect> DamageEffect;
};