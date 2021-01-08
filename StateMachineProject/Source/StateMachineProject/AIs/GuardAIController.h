#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/MetaNavMeshPath.h"
#include "InfluenceMapPropagator.h"
#include "PathfindingController.h"
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

	void Die();
	void AlertToPoint(FVector newInvestigationPoint);

	void Patrol();
	void Aim();
	void Fire();
	void Reload();
	void Flee();
	void Investigate();
	void FindHelp();

	void OnEnterAttackState();
	void OnExitAttackState();
	void OnAttackToInvestigate();

	bool IsAimingAtTarget();
	bool HasAmmo();
	bool HasLowHealth();
	bool HasHighHealth();
	bool CanSeeEnemy();
	bool IsInvestigating();
	bool IsVulnerable();
	bool IsNotVulnerable();

	bool dead = false;

private:
	AActor* actor;
	UGuardAISettings* aiSettings;
	UInfluenceMapPropagator* propagator;
	UPathfindingController* pathfindingController;
	TArray<TacticalInformation*> fleeTacticalInformation;
	UPROPERTY()
	UHSMStateMachine* fsm;

	int destIndex;
	bool moveCompleted;
	bool hasUninvestigatedLocation = false;
	FVector investigationPoint;
	bool investigating;
	float timeInvestigating;
};
