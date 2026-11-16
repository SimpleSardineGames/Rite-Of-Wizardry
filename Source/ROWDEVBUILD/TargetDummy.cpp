// Copyright Holden Woods. Rite of Wizardry.

#include "TargetDummy.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ROWAttributeSet.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

ATargetDummy::ATargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UROWAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ATargetDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ATargetDummy::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

void ATargetDummy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROWAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ATargetDummy::OnHealthChanged);
	}
}

void ATargetDummy::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!AbilitySystemComponent || !EffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void ATargetDummy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
			FString::Printf(TEXT("Dummy health: %.0f"), Data.NewValue));
	}

	if (Data.NewValue <= 0.f)
	{
		Destroy();
	}
}