#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GuardAISettings.h"
#include "HSMStateMachine.h"
#include "HSMState.h"
#include "GuardAIController.generated.h"

UCLASS()
class STATEMACHINEPROJECT_API AGuardAIController : public AAIController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	void PatrolFunction();
	bool HealthLow();

private:
	AActor* actor;
	UGuardAISettings* aiSettings;
	UHSMStateMachine* fsm;
};
