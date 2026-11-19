// Copyright Holden Woods. Rite of Wizardry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ROWShopkeeper.generated.h"

class UGameplayEffect;
class UTexture2D;
class USkeletalMeshComponent;
class USphereComponent;
class UTextRenderComponent;
class UROWShopWidget;
class AROWPlayerCharacter;

USTRUCT(BlueprintType)
struct FROWShopItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (MultiLine = "true"))
	FText Description;

	// Picture shown on the item's card in the shop menu.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 Price = 5;

	// Applied to the buyer on purchase (for example GE_Shop_Heal).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	TSubclassOf<UGameplayEffect> Effect;

	// How many times this item can be bought from this shopkeeper. 0 = unlimited.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 Stock = 1;
};

/**
 * Shopkeeper NPC. Stand next to them and press Interact (E) to open the shop menu.
 * Items, prices and stock are set in the Blueprint's Class Defaults.
 */
UCLASS()
class ROWDEVBUILD_API AROWShopkeeper : public AActor
{
	GENERATED_BODY()

public:
	AROWShopkeeper();

	// Opens the shop menu for this player.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void OpenShop(AROWPlayerCharacter* Player);

	// Closes the menu and gives control back to the player. Call from the menu's Close button.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void CloseShop();

	// Buys item ItemIndex for Buyer if it's in stock and affordable. Returns true on success.
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool TryPurchase(int32 ItemIndex, AROWPlayerCharacter* Buyer);

	UFUNCTION(BlueprintPure, Category = "Shop")
	const TArray<FROWShopItem>& GetItems() const { return Items; }

	// True if the item can still be bought (stock remaining).
	UFUNCTION(BlueprintPure, Category = "Shop")
	bool IsInStock(int32 ItemIndex) const;

protected:
	UFUNCTION()
	void OnInteractBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	// Blueprint hooks for voice lines or sounds.
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnPurchased(const FROWShopItem& Item);

	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnPurchaseFailed(const FROWShopItem& Item, bool bOutOfStock);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractSphere;

	// The shopkeeper's body. Assign a skeletal mesh (and an idle animation) in the Blueprint.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	// "Press E to shop" text, shown while the player is in range.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTextRenderComponent> Prompt;

	virtual void BeginPlay() override;

	// Every item this shopkeeper could sell. Each visit picks ItemsToShow of them at random.
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TArray<FROWShopItem> ItemPool;

	// How many cards appear in the shop (3 or 4 works well). 0 = show the whole pool.
	UPROPERTY(EditDefaultsOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 ItemsToShow = 3;

	// The items on sale this visit, picked from ItemPool at BeginPlay.
	UPROPERTY(VisibleInstanceOnly, Category = "Shop")
	TArray<FROWShopItem> Items;

	// The menu to show (WBP_Shop, parent class ROWShopWidget).
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UROWShopWidget> ShopWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UROWShopWidget> ActiveWidget;

	// Purchases made per item, matched to Items by index.
	TArray<int32> TimesBought;
};