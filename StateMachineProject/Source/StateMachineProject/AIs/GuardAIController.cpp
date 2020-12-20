#include "GuardAIController.h"
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>

void AGuardAIController::BeginPlay()
{
    Super::BeginPlay();
    actor = this->GetViewTarget();
    propagator = actor->FindComponentByClass<UInfluenceMapPropagator>();
    aiSettings = actor->FindComponentByClass<UGuardAISettings>();
    pathfindingController = UTacticalPathfindingController::FindInstanceInWorld(GetWorld());
    fleeTacticalInformation.Add(new AvoidEnemyTacticalInformation(5.0f, propagator));

    moveCompleted = true;

    //SET UP STATE MACHINE:
    fsm = UHSMStateMachine::MAKE();

    UHSMState* patrolState = UHSMState::MAKE(nullptr,
                                             [this]() { this->Patrol(); },
                                             nullptr);

    UHSMStateMachine* attackStateMachine = UHSMStateMachine::MAKE(nullptr,
                                                                  nullptr,
                                                                  [this]() { return this->OnExitAttackState(); });

    UHSMState* aimState = UHSMState::MAKE(nullptr,
                                          [this]() { this->Aim(); },
                                          nullptr);
    UHSMState* shootState = UHSMState::MAKE(nullptr,
                                            [this]() { this->Fire(); },
                                            nullptr);
    UHSMState* reloadState = UHSMState::MAKE(nullptr,
                                             [this]() { this->Reload(); },
                                             nullptr);
    UHSMState* fleeState = UHSMState::MAKE(nullptr,
                                           [this]() { this->Flee(); },
                                           nullptr);

    UHSMTransition* patrolToAttackTransition = UHSMTransition::MAKE(attackStateMachine, [this]() { return this->CanSeeEnemy(); });
    patrolState->AddTransition(patrolToAttackTransition);

    UHSMTransition* attackToPatrolTransition = UHSMTransition::MAKE(patrolState, [this]() { return !this->CanSeeEnemy(); });
    attackStateMachine->AddTransition(attackToPatrolTransition);

    UHSMTransition* aimToShootTransition = UHSMTransition::MAKE(shootState, [this]() { return this->IsAimingAtTarget(); });
    aimState->AddTransition(aimToShootTransition);

    UHSMTransition* shootToReloadTransition = UHSMTransition::MAKE(reloadState, [this]() { return !this->HasAmmo(); });
    shootState->AddTransition(shootToReloadTransition);

    UHSMTransition* shootToAimTransition = UHSMTransition::MAKE(aimState, [this]() { return !this->IsAimingAtTarget(); });
    shootState->AddTransition(shootToAimTransition);

    UHSMTransition* reloadToShootTransition = UHSMTransition::MAKE(shootState, [this]() { return this->HasAmmo(); });
    reloadState->AddTransition(reloadToShootTransition);

    attackStateMachine->AddState(aimState);
    attackStateMachine->AddState(shootState);
    attackStateMachine->AddState(reloadState);

    fsm->AddState(fleeState);
    fsm->AddState(patrolState);
    fsm->AddState(attackStateMachine);

    fsm->OnEnter();
}

void AGuardAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    fsm->OnTick();
}

void AGuardAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    moveCompleted = true;
}

float GetAngleBetweenVectors(FVector vector1, FVector vector2)
{
    vector1.Normalize();
    vector2.Normalize();
    return FMath::RadiansToDegrees(-atan2(vector1.X * vector2.Y - vector1.Y * vector2.X, vector1.X * vector2.X + vector1.Y * vector2.Y));
}

void AGuardAIController::Patrol()
{
    if (moveCompleted && propagator->GetCurrentNode() != nullptr)
    {
        //Get influence map information:
        std::vector<float> allyInfluences = std::vector<float>(propagator->GetInfluenceMap().size());
        propagator->GetInfluenceMapController()->GetPropagatorAllyInfluenceMap(propagator, allyInfluences);
        TArray<UGraphNode*> nodes = propagator->GetInfluenceMapController()->GetNodes();
        UGraphNode* currentNode = propagator->GetCurrentNode();

        TArray<UGraphNode*> prioritisedNodes;
        TArray<UGraphNode*> validNodes;
        TArray<UGraphNode*> queue;
        TArray<UGraphNode*> visitedNodes;
        queue.Add(currentNode);
        //Search for all the nodes that form the boundary of having no influence (using modified Dijkstra's)
        while (queue.Num() > 0)
        {
            currentNode = queue[0];
            TArray<UGraphNode*> neighbours;
            currentNode->GetNeighbours().GenerateKeyArray(neighbours);
         
            for (UGraphNode* neighbour : neighbours)
            {
                //If no allies have influence over the neighbour:
                if (allyInfluences[neighbour->GetIndex()] == 0)
                {
                    //Calculate the angle the agent would have to turn to face the node:
                    FVector agentForward = actor->GetActorForwardVector();
                    FVector directionToNeighbour = neighbour->GetCoordinates() - actor->GetActorLocation();
                    float angle = GetAngleBetweenVectors(agentForward, directionToNeighbour);
                    //If the node is in the agents FOV, prioritise it:
                    if (angle > -30.0f && angle < 30.0f)
                    {
                        prioritisedNodes.Add(neighbour);
                    }
                    else
                    {
                        validNodes.Add(neighbour);
                    }
                    break;
                }
                //else if the node has not been visited add it to the queue:
                else if(!visitedNodes.Contains(neighbour))
                {
                    queue.Add(neighbour);
                }
            }
            visitedNodes.Add(currentNode);
            queue.RemoveAt(0);
        }
        UGraphNode* destNode = nullptr;
        //If there are prioritised nodes, 70% chance to move to a random prioritised node:
        if (prioritisedNodes.Num() > 0 && (rand() % 100) < 70)
        {
            destNode = prioritisedNodes[rand() % prioritisedNodes.Num()];
        }
        //30% chance to move to a node out of the agents FOV:
        else
        {
           destNode = validNodes[rand() % validNodes.Num()];
        }
        MoveToLocation(destNode->GetCoordinates(), 20.0f, true, true, true, true);
        moveCompleted = false;
    }
}

void AGuardAIController::Aim()
{
    aiSettings->aiming = true;
    //Get agents forward vector, and vector to target:
    FVector gunLocation = aiSettings->gun->GetComponentLocation();
    FVector targetLocation = aiSettings->target->GetTargetLocation();
    targetLocation.Z = gunLocation.Z;
    FVector desiredDirection = targetLocation - gunLocation;
    FVector gunDirection = aiSettings->gun->GetRightVector();
    //Calculate angle between two vectors, and clamp to rotation speed:
    float angle = FMath::Clamp(GetAngleBetweenVectors(desiredDirection, gunDirection), -5.0f, 5.0f);
    //Create and add rotation to agent:
    FRotator NewRotation = FRotator(0, angle, 0);
    FQuat QuatRotation = FQuat(NewRotation);
    actor->AddActorLocalRotation(QuatRotation, false, 0, ETeleportType::None);
}

void AGuardAIController::Fire()
{
    if (!aiSettings->firing && aiSettings->aimed)
    {
        aiSettings->FireWeapon();
    }
}

void AGuardAIController::Reload()
{
    aiSettings->ReloadWeapon();
}

void AGuardAIController::Flee()
{
    UGraphNode* currentNode = propagator->GetCurrentNode();
    if (currentNode == nullptr)
    {
        return;
    }
    std::vector<float> enemyInfluenceMap = std::vector<float>(propagator->GetInfluenceMap().size());
    propagator->GetInfluenceMapController()->GetPropagatorEnemyInfluenceMap(propagator, enemyInfluenceMap);
    std::vector<float> enemyLOSMap = std::vector<float>(propagator->GetInfluenceMap().size());
    propagator->GetInfluenceMapController()->GetPropagatorEnemyLOSMap(propagator, enemyLOSMap);

    
    if ((moveCompleted && (enemyLOSMap[currentNode->GetIndex()] > 0.0f || enemyInfluenceMap[currentNode->GetIndex()] > 0.0f)) || 
        (!moveCompleted && (enemyLOSMap[destIndex] > 0.0f || enemyInfluenceMap[destIndex] > 0.0f)))
    {
        TArray<UGraphNode*> validNodes;
        TArray<UGraphNode*> queue;
        TArray<UGraphNode*> visitedNodes;
        UGraphNode* observedNode = propagator->GetCurrentNode();
        queue.Add(observedNode);
        while (queue.Num() > 0)
        {
            observedNode = queue[0];
            TArray<UGraphNode*> neighbours;
            observedNode->GetNeighbours().GenerateKeyArray(neighbours);

            for (UGraphNode* neighbour : neighbours)
            {
                //If no allies have influence over the neighbour:
                if (enemyLOSMap[neighbour->GetIndex()] <= 0.0f && enemyInfluenceMap[neighbour->GetIndex()] <= 0.0f && !validNodes.Contains(neighbour))
                {
                    bool tooClose = false;
                    for (UGraphNode* node : validNodes)
                    {
                        if (FVector::Dist(node->GetCoordinates(), neighbour->GetCoordinates()) < 150.0f)
                        {
                            tooClose = true;
                            break;
                        }
                    }
                    if (!tooClose)
                    {
                        validNodes.Add(neighbour);
                    }
                }
                //else if the node has not been visited add it to the queue:
                else if (!visitedNodes.Contains(neighbour) && !queue.Contains(neighbour))
                {
                    queue.Add(neighbour);
                }
            }
            visitedNodes.Add(observedNode);
            queue.RemoveAt(0);
        }
        //Find the shortest path to a point of cover:
        TArray<UPathNode*> shortestPath;
        float shortestPathLength = INT_MAX;
        for (UGraphNode* validNode : validNodes)
        {
            TArray<UPathNode*> path = pathfindingController->RunPathfinding(currentNode->GetIndex(), validNode->GetIndex(), fleeTacticalInformation);
            float pathLength = pathfindingController->CalculatePathLength(path, fleeTacticalInformation);
            UNavigationPath* pathToEnemy = UNavigationSystemV1::GetCurrent(GetWorld())->FindPathToLocationSynchronously(GetWorld(), aiSettings->target->GetActorLocation(), validNode->GetCoordinates());
            if (pathLength - pathToEnemy->GetPathLength() < shortestPathLength)
            {
                shortestPath = path;
                shortestPathLength = pathLength;
            }
        }
        UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent();
        //Add all the waypoints from the calculated path:
        TArray<FVector> locations;
        for (UPathNode* pathNode : shortestPath)
        {
            locations.Add(pathNode->node->GetCoordinates());
        }
        //Assign the new path to the agent:
        AController* controller = Cast<AController>(this);
        FMetaNavMeshPath* MetaNavMeshPath = new FMetaNavMeshPath(locations, *controller);
        TSharedPtr<FMetaNavMeshPath, ESPMode::ThreadSafe> MetaPathPtr(MetaNavMeshPath);
        pathFollowingComponent->RequestMove(FAIMoveRequest(), MetaPathPtr);
        moveCompleted = false;
    }
}

void AGuardAIController::OnExitAttackState()
{
    aiSettings->aiming = false;
}

bool AGuardAIController::IsAimingAtTarget()
{
    //Fire raycast out of gun barrel:
    TArray<FHitResult> hitResults;
    FVector start = aiSettings->gun->GetComponentLocation();
    FVector end = start + (2000000.0f * aiSettings->gun->GetRightVector());
    FCollisionQueryParams collisionParams;
    FCollisionResponseParams collisionResponseParams = FCollisionResponseParams(ECollisionResponse::ECR_Overlap);
    GetWorld()->LineTraceMultiByChannel(hitResults, start, end, ECC_WorldDynamic, collisionParams, collisionResponseParams);
    //Loop over hit results:
    for (FHitResult hitResult : hitResults)
    {
        //If it hits the target before it hits anything else, it is aiming at the target:
        if (hitResult.Actor == actor)
        {
            continue;
        }
        else if (hitResult.Actor == aiSettings->target)
        {
            return true;
        }
    }
    return false;
}

bool AGuardAIController::HasAmmo()
{
    return aiSettings->ammo > 0;
}

bool AGuardAIController::CanSeeEnemy()
{
    //Calculate angle between agents forward vector and vector to target:
    FVector dir = aiSettings->target->GetTargetLocation() - actor->GetTargetLocation();
    FVector actorForward = actor->GetActorForwardVector();
    float angle = GetAngleBetweenVectors(dir, actorForward);
    //Calculate distance to target:
    float dist = FVector::Dist(actor->GetTargetLocation(), aiSettings->target->GetTargetLocation());
    //Fire raycast between agent and target:
    TArray<FHitResult> hitResults;
    FCollisionQueryParams collisionParams;
    FCollisionResponseParams collisionResponseParams = FCollisionResponseParams(ECollisionResponse::ECR_Overlap);
    GetWorld()->LineTraceMultiByChannel(hitResults, actor->GetTargetLocation(), aiSettings->target->GetTargetLocation(), ECC_WorldDynamic, collisionParams, collisionResponseParams);
    //If the raycast hits anything that is not the target, it is obstructured:
    bool obstructed = false;
    for (FHitResult hitResult : hitResults)
    {
        if (hitResult.Actor == actor)
        {
            continue;
        }
        else if (hitResult.Actor != aiSettings->target)
        {
            obstructed = true;
            break;
        }
    } 
    //If the target is in the field of view, and not obstructed it is visible to the agent:
    return (angle >= -80 && angle <= 80 && !obstructed);
}
