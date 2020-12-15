#include "GuardAIController.h"

void AGuardAIController::BeginPlay()
{
    Super::BeginPlay();
    actor = this->GetViewTarget();
    aiSettings = actor->FindComponentByClass<UGuardAISettings>();
    Super::MoveToLocation(aiSettings->waypoints[aiSettings->currentWaypointIndex], 20.0f, true, true, true, true);

    //SET UP STATE MACHINE:
    fsm = UHSMStateMachine::MAKE();

    UHSMState* patrolState = UHSMState::MAKE(nullptr,
                                             [this]() { this->PatrolFunction(); },
                                             nullptr);
    UHSMState* fleeState = UHSMState::MAKE();

    UHSMTransition* patrolToFleeTransition = UHSMTransition::MAKE(fleeState, [this]() { return this->HealthLow(); });
    patrolState->AddTransition(patrolToFleeTransition);

    fsm->AddState(patrolState);
    fsm->AddState(fleeState);

    fsm->OnEnter();
}

void AGuardAIController::PatrolFunction()
{
    FString objectName = actor->GetName();
    UE_LOG(LogTemp, Warning, TEXT("%s is Patrolling"), *objectName);
}

bool AGuardAIController::HealthLow()
{
    UE_LOG(LogTemp, Warning, TEXT("Health High"));
    return false;
}

void AGuardAIController::Tick(float DeltaSeconds)
{
    fsm->OnTick();
}

void AGuardAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    if (++(aiSettings->currentWaypointIndex) < aiSettings->waypoints.Num())
    {
        //Increment waypoint, loop around to 0 if goes beyond score of waypoint list:
        //Move to new Waypoint:
        Super::MoveToLocation(aiSettings->waypoints[aiSettings->currentWaypointIndex], 20.0f, true, true, true, true);
    }
}