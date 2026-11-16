// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ROWAttributeSet.generated.h"

// Generates Get/Set/Init functions for each attribute.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Core combat attributes for anything that can take damage.
 * Damage is a "meta" attribute: Gameplay Effects write incoming damage into it,
 * and PostGameplayEffectExecute converts it into a Health reduction.
 */
UCLASS()
class ROWDEVBUILD_API UROWAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UROWAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UROWAttributeSet, Health)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UROWAttributeSet, MaxHealth)

		// Meta attribute: incoming damage, consumed in PostGameplayEffectExecute.
		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UROWAttributeSet, Damage)

		virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
