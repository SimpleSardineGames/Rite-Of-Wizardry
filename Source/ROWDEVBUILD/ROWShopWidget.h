// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ROWShopWidget.generated.h"

class AROWShopkeeper;
class AROWPlayerCharacter;

/**
 * Parent class for the shop menu widget (WBP_Shop). The shopkeeper fills in Shopkeeper and Buyer,
 * then calls OnShopOpened, where the Blueprint builds the item list.
 */
UCLASS(Abstract)
class ROWDEVBUILD_API UROWShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TObjectPtr<AROWShopkeeper> Shopkeeper;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TObjectPtr<AROWPlayerCharacter> Buyer;

	// Build or refresh the item list here.
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnShopOpened();
};