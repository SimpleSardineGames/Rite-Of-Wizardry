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
 * Spell projectile base (Fireball, Frost, Void are Blueprint children with different settings).
 * Flies straight along its forward vector. On hitting something it applies DamageEffect plus any
 * ExtraEffects to targets with an Ability System Component. If ExplosionRadius > 0 it hits every
 * target in that radius instead of only the one it touched.
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

	// Applies DamageEffect and ExtraEffects to one actor, if it has an Ability System Component.
	void ApplyEffectsTo(AActor* Target);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> Sphere;

	// Visual only; assign a mesh in the Blueprint child.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// Gameplay Effect applied to whatever the spell hits (for example GE_Damage_Test).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// Additional effects applied on hit, such as a slow for Frost.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell")
	TArray<TSubclassOf<UGameplayEffect>> ExtraEffects;

	// 0 = single target. Above 0 = damages every target within this radius of the impact (Void).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "0"))
	float ExplosionRadius = 0.f;
};