// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "TargetDummy.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UROWAttributeSet;
class UStaticMeshComponent;
struct FOnAttributeChangeData;

/**
 * A simple target that owns an Ability System Component and the ROW attribute set,
TESTING ONLY
 */
UCLASS()
class ROWDEVBUILD_API ATargetDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATargetDummy();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealth() const;

	// Applies a Gameplay Effect (for example a damage effect) to this dummy.
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass);

protected:
	virtual void BeginPlay() override;

	void OnHealthChanged(const FOnAttributeChangeData& Data);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UROWAttributeSet> AttributeSet;
};