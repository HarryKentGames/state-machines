#include "GuardAISettings.h"
#include "GuardAIController.h"
#include "Kismet/GameplayStatics.h"

UGuardAISettings::UGuardAISettings()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UGuardAISettings::BeginPlay()
{
	Super::BeginPlay();	
	firing = false;

	TArray<UActorComponent*> children = TArray<UActorComponent*>();
	children = GetOwner()->GetComponentsByClass(UActorComponent::StaticClass());
	for (UActorComponent* child : children)
	{
		if (child->GetName() == "Gun")
		{
			gun = (USceneComponent*) child;
		}
	}
	ammo = 10;
}

void UGuardAISettings::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	timeSinceTakenDamage += DeltaTime;
	if (timeSinceTakenDamage > 5 && health < maxHealth)
	{
		health += DeltaTime * 5;
		if (health > maxHealth)
		{
			health = maxHealth;
		}
	}
}

void UGuardAISettings::FireWeapon()
{
	firing = true; 
	ammo--;
	DrawDebugLine(GetWorld(), gun->GetComponentLocation(), target->GetTargetLocation(), FColor::Red, false, 0.09f, (uint8)'\000', 5.0f);
	target->FindComponentByClass<UGuardAISettings>()->TakeDamage(3);
	for (int i = 0; i < enemies.Num(); i++)
	{
		AActor* enemy = enemies[i];
		AGuardAIController* enemyController = (AGuardAIController*)((APawn*)enemy)->GetController();
		enemyController->SetInvestigationPoint(GetOwner()->GetActorLocation());
	}
}

void UGuardAISettings::ReloadWeapon()
{
	reloading = true;
	ammo = 10;
}

void UGuardAISettings::TakeDamage(float damage)
{
	health -= damage;
	if (health <= 0)
	{
		AGuardAIController* controller = (AGuardAIController*)((APawn*)GetOwner())->GetController();
		if (controller != nullptr && !controller->dead)
		{
			controller->Die();
		}
	}
	timeSinceTakenDamage = 0;
}

