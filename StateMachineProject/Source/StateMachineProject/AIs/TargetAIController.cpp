#include "TargetAIController.h"

void ATargetAIController::BeginPlay()
{
    Super::BeginPlay();
    actor = this->GetViewTarget();
    aiSettings = actor->FindComponentByClass<UTargetAISettings>();
    Super::MoveToLocation(aiSettings->waypoints[aiSettings->currentWaypointIndex], 20.0f, true, true, true, true);
}

void ATargetAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    if (++(aiSettings->currentWaypointIndex) < aiSettings->waypoints.Num())
    {
        //Increment waypoint, loop around to 0 if goes beyond score of waypoint list:
        //Move to new Waypoint:
        Super::MoveToLocation(aiSettings->waypoints[aiSettings->currentWaypointIndex], 20.0f, true, true, true, true);
    }
    if (aiSettings->currentWaypointIndex >= aiSettings->waypoints.Num())
    {
        aiSettings->currentWaypointIndex = 0;
        Super::MoveToLocation(aiSettings->waypoints[aiSettings->currentWaypointIndex], 20.0f, true, true, true, true);
    }
}