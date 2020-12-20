#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetAISettings.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STATEMACHINEPROJECT_API UTargetAISettings : public UActorComponent
{
	GENERATED_BODY()

public:	
	UTargetAISettings();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	TArray<FVector> waypoints;
	int currentWaypointIndex = 0;

protected:
	virtual void BeginPlay() override;		
};
