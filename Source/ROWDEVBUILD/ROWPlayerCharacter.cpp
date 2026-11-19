// Copyright Holden Woods. Rite of Wizardry.

#include "ROWPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "ROWAttributeSet.h"
#include "ROWShopkeeper.h"
#include "ROWFireball.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"

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

	OnPlayerHealthChanged.Broadcast(GetHealth(), GetMaxHealth());

	Coins = StartingCoins;
	OnPlayerCoinsChanged.Broadcast(Coins);

	GetWorldTimerManager().SetTimer(ManaRegenTimer, this, &AROWPlayerCharacter::RegenMana, ManaRegenInterval, true);

	// Menus (game over, main menu) switch input to "UI only", and that setting survives a level reload.
	// Always hand control back to the game when the wizard spawns.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		EnableInput(PC);
	}
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

		if (bRagdollOnDeath)
		{
			StartRagdoll();
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

float AROWPlayerCharacter::GetMana() const
{
	return AttributeSet ? AttributeSet->GetMana() : 0.f;
}

float AROWPlayerCharacter::GetMaxMana() const
{
	return AttributeSet ? AttributeSet->GetMaxMana() : 0.f;
}

void AROWPlayerCharacter::RegenMana()
{
	if (bIsDead || !AbilitySystemComponent || GetMana() >= GetMaxMana())
	{
		return;
	}

	const float NewMana = FMath::Min(GetMana() + ManaRegenPerSecond * ManaRegenInterval, GetMaxMana());
	AbilitySystemComponent->SetNumericAttributeBase(UROWAttributeSet::GetManaAttribute(), NewMana);
}

bool AROWPlayerCharacter::TryCastSpell(TSubclassOf<AROWFireball> SpellClass, FVector Direction)
{
	if (bIsDead || !SpellClass || !AbilitySystemComponent)
	{
		return false;
	}

	// Each spell's Mana Cost lives on its Blueprint's defaults.
	const float Cost = SpellClass->GetDefaultObject<AROWFireball>()->GetManaCost();
	if (GetMana() < Cost)
	{
		OnOutOfMana();
		return false;
	}

	Direction.Z = 0.f;
	if (!Direction.Normalize())
	{
		Direction = GetActorForwardVector();
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Location = GetActorLocation() + Direction * SpellSpawnDistance;
	if (!GetWorld()->SpawnActor<AROWFireball>(SpellClass, Location, Direction.Rotation(), Params))
	{
		return false;
	}

	AbilitySystemComponent->SetNumericAttributeBase(UROWAttributeSet::GetManaAttribute(), GetMana() - Cost);
	return true;
}

void AROWPlayerCharacter::StartRagdoll()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body || !Body->GetPhysicsAsset())
	{
		return; // No Physics Asset on the mesh: nothing to simulate.
	}

	// The capsule stops colliding so the body can fall freely. The camera stays attached to it.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Switch the mesh from animation to physics.
	Body->SetCollisionProfileName(TEXT("Ragdoll"));
	Body->SetAllBodiesSimulatePhysics(true);
	Body->SetSimulatePhysics(true);
	Body->WakeAllRigidBodies();
	Body->bBlendPhysics = true;

	// A small shove backwards so the fall reads clearly from the top-down camera.
	if (DeathImpulse > 0.f)
	{
		Body->AddImpulse(-GetActorForwardVector() * DeathImpulse + FVector(0.f, 0.f, DeathImpulse * 0.3f), NAME_None, true);
	}
}

void AROWPlayerCharacter::RestartLevel()
{
	APlayerController* PC = Cast<APlayerController>(GetController());

	// Death screen: show it, hand the mouse to the menu, and freeze the world behind it.
	if (GameOverWidgetClass && PC)
	{
		if (UUserWidget* GameOver = CreateWidget<UUserWidget>(PC, GameOverWidgetClass))
		{
			GameOver->AddToViewport(100);

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(GameOver->TakeWidget());
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
			UGameplayStatics::SetGamePaused(this, true);
			return;
		}
	}

	// No death screen set: restart straight away.
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}