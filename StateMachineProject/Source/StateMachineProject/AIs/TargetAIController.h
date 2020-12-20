#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TargetAISettings.h"
#include "TargetAIController.generated.h"

UCLASS()
class STATEMACHINEPROJECT_API ATargetAIController : public AAIController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	AActor* actor;
	UTargetAISettings* aiSettings;
};
