// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Navigation/MetaNavMeshPath.h"
#include "InfluenceMapPropagator.h"
#include "PathfindingController.h"
#include "SoldierAIConfig.h"
#include "HSMStateMachine.h"
#include "HSMState.h"
#include "SoldierAIController.generated.h"

UCLASS()
class STATEMACHINEPROJECT_API ASoldierAIController : public AAIController
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
	void OnEnterInvestigateState();

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
	USoldierAIConfig* aiSettings;
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
