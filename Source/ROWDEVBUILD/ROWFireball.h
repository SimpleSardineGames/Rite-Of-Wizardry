// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWFireball.generated.h"

class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTexture2D;

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

	UFUNCTION(BlueprintPure, Category = "Spell")
	float GetManaCost() const { return ManaCost; }

	const FText& GetSpellName() const { return SpellName; }
	UTexture2D* GetSpellIcon() const { return SpellIcon; }

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

	// Damage effect applied to whatever the spell hits. Use GE_Spell_Damage, whose Damage modifier
	// is "Set By Caller" with the tag Data.Damage: the spell fills it with BaseDamage x the caster's SpellPower.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// Shown on the spell bar.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell")
	FText SpellName;

	// Shown on the spell bar.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell")
	TObjectPtr<UTexture2D> SpellIcon;

	// Mana spent to cast this spell (0 = free; leave boss bolts at 0).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "0"))
	float ManaCost = 0.f;

	// Damage before the caster's SpellPower multiplier.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "0"))
	float BaseDamage = 25.f;

	// Additional effects applied on hit, such as a slow for Frost.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell")
	TArray<TSubclassOf<UGameplayEffect>> ExtraEffects;

	// 0 = single target. Above 0 = damages every target within this radius of the impact (Void).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell", meta = (ClampMin = "0"))
	float ExplosionRadius = 0.f;
};