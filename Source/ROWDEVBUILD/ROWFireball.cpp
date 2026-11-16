// Copyright Holden Woods. Rite of Wizardry.

#include "ROWFireball.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AROWFireball::AROWFireball()
{
	PrimaryActorTick.bCanEverTick = false;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->InitSphereRadius(30.f);
	// "BlockAllDynamic" ships with every project and makes the fireball generate hit events.
	Sphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Sphere->OnComponentHit.AddDynamic(this, &AROWFireball::OnHit);
	SetRootComponent(Sphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = Sphere;
	ProjectileMovement->InitialSpeed = 1500.f;
	ProjectileMovement->MaxSpeed = 1500.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	// Destroy automatically if it never hits anything.
	InitialLifeSpan = 3.f;
}

void AROWFireball::BeginPlay()
{
	Super::BeginPlay();

	// Don't collide with whoever cast the fireball.
	if (AActor* OwnerActor = GetOwner())
	{
		Sphere->IgnoreActorWhenMoving(OwnerActor, true);
	}
}

void AROWFireball::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// If the target has an Ability System Component, apply the damage effect to it.
	if (DamageEffect)
	{
		if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(OtherActor))
		{
			if (UAbilitySystemComponent* TargetASC = TargetInterface->GetAbilitySystemComponent())
			{
				FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
				Context.AddSourceObject(this);

				const FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
				if (Spec.IsValid())
				{
					TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
		}
	}

	// Fireballs are used up by any hit, including walls.
	Destroy();
}