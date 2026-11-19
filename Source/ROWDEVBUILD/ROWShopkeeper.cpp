// Copyright Holden Woods. Rite of Wizardry.

#include "ROWShopkeeper.h"
#include "ROWShopWidget.h"
#include "ROWPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"

AROWShopkeeper::AROWShopkeeper()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->InitSphereRadius(250.f);
	InteractSphere->SetCollisionProfileName(TEXT("Trigger"));
	InteractSphere->SetGenerateOverlapEvents(true);
	InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &AROWShopkeeper::OnInteractBeginOverlap);
	InteractSphere->OnComponentEndOverlap.AddDynamic(this, &AROWShopkeeper::OnInteractEndOverlap);
	SetRootComponent(InteractSphere);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(InteractSphere);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	Prompt = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Prompt"));
	Prompt->SetupAttachment(InteractSphere);
	Prompt->SetText(FText::FromString(TEXT("Press E to shop")));
	Prompt->SetHorizontalAlignment(EHTA_Center);
	Prompt->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
	Prompt->SetWorldSize(40.f);
	Prompt->SetHiddenInGame(true);
}

void AROWShopkeeper::BeginPlay()
{
	Super::BeginPlay();

	// Pick this visit's stock at random from the pool, with no repeats.
	TArray<FROWShopItem> Pool = ItemPool;
	const int32 Count = (ItemsToShow <= 0) ? Pool.Num() : FMath::Min(ItemsToShow, Pool.Num());

	Items.Reset();
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Pick = FMath::RandRange(0, Pool.Num() - 1);
		Items.Add(Pool[Pick]);
		Pool.RemoveAtSwap(Pick);
	}

	TimesBought.SetNumZeroed(Items.Num());
}

void AROWShopkeeper::OnInteractBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AROWPlayerCharacter>(OtherActor))
	{
		Prompt->SetHiddenInGame(false);
	}
}

void AROWShopkeeper::OnInteractEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (Cast<AROWPlayerCharacter>(OtherActor))
	{
		Prompt->SetHiddenInGame(true);
		CloseShop();
	}
}

void AROWShopkeeper::OpenShop(AROWPlayerCharacter* Player)
{
	if (!Player || !ShopWidgetClass || ActiveWidget)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC)
	{
		return;
	}

	ActiveWidget = CreateWidget<UROWShopWidget>(PC, ShopWidgetClass);
	if (!ActiveWidget)
	{
		return;
	}

	ActiveWidget->Shopkeeper = this;
	ActiveWidget->Buyer = Player;
	ActiveWidget->AddToViewport(10);
	ActiveWidget->OnShopOpened();

	// Hand input to the menu: mouse cursor on, character controls off.
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	Player->DisableInput(PC);
}

void AROWShopkeeper::CloseShop()
{
	if (!ActiveWidget)
	{
		return;
	}

	APlayerController* PC = ActiveWidget->GetOwningPlayer();
	AROWPlayerCharacter* Player = ActiveWidget->Buyer;

	ActiveWidget->RemoveFromParent();
	ActiveWidget = nullptr;

	if (PC)
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		if (Player)
		{
			Player->EnableInput(PC);
		}
	}
}

bool AROWShopkeeper::IsInStock(int32 ItemIndex) const
{
	if (!Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	const int32 Bought = TimesBought.IsValidIndex(ItemIndex) ? TimesBought[ItemIndex] : 0;
	return Items[ItemIndex].Stock == 0 || Bought < Items[ItemIndex].Stock;
}

// Warns about setup mistakes (missing Buyer, missing Effect). Normal outcomes like
// "not enough coins" go to OnPurchaseFailed instead, for the UI to show.
static void ShopDebugMessage(const FString& Message)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("Shop: ") + Message);
	}
}

bool AROWShopkeeper::TryPurchase(int32 ItemIndex, AROWPlayerCharacter* Buyer)
{
	if (!Buyer)
	{
		ShopDebugMessage(TEXT("no Buyer passed in. Check the card's Buyer pin is connected."));
		return false;
	}

	if (!Items.IsValidIndex(ItemIndex))
	{
		ShopDebugMessage(FString::Printf(TEXT("item index %d doesn't exist (shop has %d items)."), ItemIndex, Items.Num()));
		return false;
	}

	const FROWShopItem& Item = Items[ItemIndex];

	if (!IsInStock(ItemIndex))
	{
		OnPurchaseFailed(Item, true);
		return false;
	}

	UAbilitySystemComponent* BuyerASC = Buyer->GetAbilitySystemComponent();
	if (!BuyerASC)
	{
		ShopDebugMessage(TEXT("the buyer has no Ability System Component."));
		return false;
	}

	if (!Item.Effect)
	{
		ShopDebugMessage(FString::Printf(TEXT("%s has no Effect set in the Item Pool."), *Item.Name.ToString()));
		return false;
	}

	// Takes the coins only if the player can afford it.
	if (!Buyer->SpendCoins(Item.Price))
	{
		OnPurchaseFailed(Item, false);
		return false;
	}

	FGameplayEffectContextHandle Context = BuyerASC->MakeEffectContext();
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = BuyerASC->MakeOutgoingSpec(Item.Effect, 1.f, Context);
	if (Spec.IsValid())
	{
		BuyerASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	if (TimesBought.Num() < Items.Num())
	{
		TimesBought.SetNumZeroed(Items.Num());
	}
	++TimesBought[ItemIndex];

	OnPurchased(Item);
	return true;
}