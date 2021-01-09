#include "SoldierAIController.h"
#include "Kismet/GameplayStatics.h"
#include "SoldierAIConfig.h"

USoldierAIConfig::USoldierAIConfig()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USoldierAIConfig::BeginPlay()
{
	Super::BeginPlay();
	firing = false;

	TArray<UActorComponent*> children = TArray<UActorComponent*>();
	children = GetOwner()->GetComponentsByClass(UActorComponent::StaticClass());
	for (UActorComponent* child : children)
	{
		if (child->GetName() == "Gun")
		{
			gun = (USceneComponent*)child;
		}
	}
	ammo = 10;
}

void USoldierAIConfig::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	timeSinceTakenDamage += DeltaTime;
	if (timeSinceTakenDamage > 3 && health < maxHealth)
	{
		health += DeltaTime * 10;
		if (health > maxHealth)
		{
			health = maxHealth;
		}
	}
}

void USoldierAIConfig::FireWeapon()
{
	firing = true;
	ammo--;
	DrawDebugLine(GetWorld(), gun->GetComponentLocation(), target->GetTargetLocation(), FColor::Red, false, 0.09f, (uint8)'\000', 5.0f);
	target->FindComponentByClass<USoldierAIConfig>()->TakeDamage(3);
	for (int i = 0; i < enemies.Num(); i++)
	{
		AActor* enemy = enemies[i];
		ASoldierAIController* enemyController = (ASoldierAIController*)((APawn*)enemy)->GetController();
		if (GetOwner() != nullptr && enemyController != nullptr)
		{
			enemyController->AlertToPoint(GetOwner()->GetActorLocation());
		}
	}
}

void USoldierAIConfig::ReloadWeapon()
{
	reloading = true;
	ammo = 10;
}

void USoldierAIConfig::TakeDamage(float damage)
{
	health -= damage;
	if (health <= 0)
	{
		ASoldierAIController* controller = (ASoldierAIController*)((APawn*)GetOwner())->GetController();
		if (controller != nullptr && !controller->dead)
		{
			controller->Die();
		}
	}
	timeSinceTakenDamage = 0;
}

