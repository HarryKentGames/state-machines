#include "GuardAISettings.h"

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
}

void UGuardAISettings::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (currentWaypointIndex >= waypoints.Num() && !firing && aimed)
	{
		FireWeapon();
	}
	else if (currentWaypointIndex >= waypoints.Num() && !aimed)
	{
		AimWeapon();
	}

	if (triggerPulled)
	{
		ShootBullet();
	}
}

void UGuardAISettings::AimWeapon()
{
	aiming = true;
}

void UGuardAISettings::FireWeapon()
{
	firing = true; 
}

void UGuardAISettings::ShootBullet()
{
	DrawDebugLine(GetWorld(), gun->GetComponentLocation(), gun->GetComponentLocation() + (2000000.0f * gun->GetRightVector()), FColor::Red, false, 0.09f, (uint8)'\000', 5.0f);
	triggerPulled = false;
}

