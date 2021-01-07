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
	float health;
	UPROPERTY(EditAnywhere)
	float maxHealth;
	float timeSinceTakenDamage;

	UPROPERTY(EditAnywhere)
	TArray<FVector> waypoints;
	int currentWaypointIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool aiming;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool aimed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool firing;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool triggerPulled;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Soldier")
	bool reloading;

	UPROPERTY(EditAnywhere)
	int ammo;

	UPROPERTY(EditAnywhere)
	USceneComponent* gun;

	UPROPERTY(EditAnywhere)
	AActor* target;

	UPROPERTY(EditAnywhere)
	TArray<AActor*> enemies;

	UGuardAISettings();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void FireWeapon();
	void ReloadWeapon();
	void TakeDamage(float damage);

protected:
	virtual void BeginPlay() override;	
};
