// Copyright Holden Woods. Rite of Wizardry.

#include "ROWPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "ROWAttributeSet.h"
#include "ROWShopkeeper.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

AROWPlayerCharacter::AROWPlayerCharacter()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UROWAttributeSet>(TEXT("AttributeSet"));

	// Top-down facing: turn toward the direction of movement, not the controller.
	// A plain ACharacter defaults to the opposite, which stops the wizard rotating.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
}

UAbilitySystemComponent* AROWPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AROWPlayerCharacter::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float AROWPlayerCharacter::GetMaxHealth() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

void AROWPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROWAttributeSet::GetHealthAttribute())
			.AddUObject(this, &AROWPlayerCharacter::OnHealthChanged);

		// So the health bar updates when Max Health Up is bought.
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UROWAttributeSet::GetMaxHealthAttribute())
			.AddUObject(this, &AROWPlayerCharacter::OnMaxHealthChanged);
	}

	// Show the starting health so it's easy to confirm the setup works.
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
			FString::Printf(TEXT("Player health: %.0f"), GetHealth()));
	}

	OnPlayerHealthChanged.Broadcast(GetHealth(), GetMaxHealth());

	Coins = StartingCoins;
	OnPlayerCoinsChanged.Broadcast(Coins);
}

void AROWPlayerCharacter::AddCoins(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Coins += Amount;
	OnPlayerCoinsChanged.Broadcast(Coins);
}

bool AROWPlayerCharacter::SpendCoins(int32 Amount)
{
	if (Amount < 0 || Coins < Amount)
	{
		return false;
	}

	Coins -= Amount;
	OnPlayerCoinsChanged.Broadcast(Coins);
	return true;
}

void AROWPlayerCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
			FString::Printf(TEXT("Player health: %.0f"), Data.NewValue));
	}

	OnPlayerHealthChanged.Broadcast(Data.NewValue, GetMaxHealth());

	if (Data.NewValue <= 0.f && !bIsDead)
	{
		bIsDead = true;

		// Stop the player from moving or casting while dead.
		GetCharacterMovement()->DisableMovement();
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}

		OnDied();

		GetWorldTimerManager().SetTimer(RestartTimer, this, &AROWPlayerCharacter::RestartLevel, RestartDelay, false);
	}
}

void AROWPlayerCharacter::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	OnPlayerHealthChanged.Broadcast(GetHealth(), Data.NewValue);
}

void AROWPlayerCharacter::TryInteract()
{
	if (bIsDead)
	{
		return;
	}

	TArray<AActor*> Shopkeepers;
	GetOverlappingActors(Shopkeepers, AROWShopkeeper::StaticClass());

	if (Shopkeepers.Num() > 0)
	{
		if (AROWShopkeeper* Shopkeeper = Cast<AROWShopkeeper>(Shopkeepers[0]))
		{
			Shopkeeper->OpenShop(this);
		}
	}
}

void AROWPlayerCharacter::RestartLevel()
{
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}