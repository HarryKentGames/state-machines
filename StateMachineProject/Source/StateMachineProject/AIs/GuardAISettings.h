#pragma once

#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "Components/ActorComponent.h"
#include "GuardAISettings.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STATEMACHINEPROJECT_API UGuardAISettings : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPROPERTY(EditAnywhere)
	TArray<FVector> waypoints;
	int currentWaypointIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool aiming;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool aimed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool firing;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool triggerPulled;

	UPROPERTY(EditAnywhere)
	USceneComponent* gun;

	UGuardAISettings();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void AimWeapon();
	void FireWeapon();
	void ShootBullet();

protected:
	virtual void BeginPlay() override;	
};
