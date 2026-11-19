// Copyright Holden Woods. Rite of Wizardry.

#include "ROWFireball.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "ROWAttributeSet.h"
#include "ROWGameplayTags.h"
#include "ROWEnemy.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

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

	if (ExplosionRadius > 0.f)
	{
		// Area spell: hit every pawn within the radius of the impact point.
		TArray<FOverlapResult> Overlaps;
		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		if (GetOwner())
		{
			QueryParams.AddIgnoredActor(GetOwner());
		}
		if (GetInstigator())
		{
			QueryParams.AddIgnoredActor(GetInstigator());
		}

		GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams,
			FCollisionShape::MakeSphere(ExplosionRadius), QueryParams);

		// One actor can show up once per component; only hit each actor once.
		TSet<AActor*> AlreadyHit;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (Target && !AlreadyHit.Contains(Target))
			{
				AlreadyHit.Add(Target);
				ApplyEffectsTo(Target);
			}
		}
	}
	else
	{
		ApplyEffectsTo(OtherActor);
	}

	// Spells are used up by any hit, including walls.
	Destroy();
}

void AROWFireball::ApplyEffectsTo(AActor* Target)
{
	// Never hurt the caster, even inside its own explosion.
	if (!Target || Target == GetOwner() || Target == GetInstigator())
	{
		return;
	}

	// No friendly fire: enemy projectiles (boss, ranged enemies) only hurt the player.
	if (Cast<AROWEnemy>(GetOwner()) && Cast<AROWEnemy>(Target))
	{
		return;
	}

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target);
	if (!TargetInterface)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetInterface->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);

	// Damage = BaseDamage x the caster's SpellPower, passed to the effect as a Set By Caller value.
	if (DamageEffect)
	{
		float SpellPower = 1.f;
		if (IAbilitySystemInterface* CasterInterface = Cast<IAbilitySystemInterface>(GetOwner()))
		{
			if (UAbilitySystemComponent* CasterASC = CasterInterface->GetAbilitySystemComponent())
			{
				bool bFound = false;
				const float Value = CasterASC->GetGameplayAttributeValue(UROWAttributeSet::GetSpellPowerAttribute(), bFound);
				if (bFound)
				{
					SpellPower = Value;
				}
			}
		}

		const FGameplayEffectSpecHandle DamageSpec = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
		if (DamageSpec.IsValid())
		{
			DamageSpec.Data->SetSetByCallerMagnitude(ROWTags::Data_Damage, BaseDamage * SpellPower);
			TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());
		}
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : ExtraEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		const FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}