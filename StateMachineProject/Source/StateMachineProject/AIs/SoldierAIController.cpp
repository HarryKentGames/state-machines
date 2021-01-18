#include "SoldierAIController.h"
#include <Runtime/Engine/Classes/Kismet/KismetMathLibrary.h>

void ASoldierAIController::BeginPlay()
{
    Super::BeginPlay();
    actor = this->GetViewTarget();
    propagator = actor->FindComponentByClass<UInfluenceMapPropagator>();
    aiConfig = actor->FindComponentByClass<USoldierAIConfig>();
    pathfindingController = UPathfindingController::FindInstanceInWorld(GetWorld());
    fleeTacticalInformation.Add(new AvoidEnemyTacticalInformation(5.0f, propagator));

    moveCompleted = true;

    //SET UP STATE MACHINE:
    fsm = UHSMStateMachine::MAKE();

    UHSMState* patrolState = UHSMState::MAKE(nullptr,
        [this]() { this->Patrol(); },
        nullptr);

    UHSMStateMachine* attackStateMachine = UHSMStateMachine::MAKE([this]() { this->OnEnterAttackState(); },
        nullptr,
        [this]() { this->OnExitAttackState(); });

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
    UHSMState* investigateState = UHSMState::MAKE([this]() { this->OnEnterInvestigateState(); },
        [this]() { this->Investigate(); },
        nullptr);
    UHSMState* findHelpState = UHSMState::MAKE(nullptr,
        [this]() { this->FindHelp(); },
        nullptr);

    //PATROL STATE TRANSITIONS:
    UHSMTransition* patrolToAttackTransition = UHSMTransition::MAKE(attackStateMachine, [this]() { return !this->HasLowHealth() && this->CanSeeEnemy() ; });
    patrolState->AddTransition(patrolToAttackTransition);

    UHSMTransition* patrolToFleeTransition = UHSMTransition::MAKE(fleeState, [this]() { return this->HasLowHealth() && this->CanSeeEnemy(); });
    patrolState->AddTransition(patrolToFleeTransition);

    UHSMTransition* patrolToInvestigateTransition = UHSMTransition::MAKE(investigateState, [this]() { return hasUninvestigatedLocation; });
    patrolState->AddTransition(patrolToInvestigateTransition);

    UHSMTransition* patrolToFindHelpTransition = UHSMTransition::MAKE(findHelpState, [this]() { return this->IsVulnerable(); });
    patrolState->AddTransition(patrolToFindHelpTransition);

    //ATTACK STATE TRANSITIONS:
    UHSMTransition* attackToPatrolTransition = UHSMTransition::MAKE(patrolState, [this]() { return aiConfig->target == nullptr && !aiConfig->reloading; });
    attackStateMachine->AddTransition(attackToPatrolTransition);

    UHSMTransition* attackToInvestigateTransition = UHSMTransition::MAKE(investigateState, [this]() { return (aiConfig->target != nullptr && !this->CanSeeEnemy()) && !aiConfig->reloading; }, [this]() { this->OnAttackToInvestigate(); });
    attackStateMachine->AddTransition(attackToInvestigateTransition);

    UHSMTransition* attackToFleeTransition = UHSMTransition::MAKE(fleeState, [this]() { return this->HasLowHealth() && !aiConfig->reloading; });
    attackStateMachine->AddTransition(attackToFleeTransition);

    UHSMTransition* attackToFindHelpTransition = UHSMTransition::MAKE(findHelpState, [this]() { return this->IsVulnerable() && !aiConfig->reloading; });
    attackStateMachine->AddTransition(attackToFindHelpTransition);

    //AIM STATE TRANSITIONS:
    UHSMTransition* aimToShootTransition = UHSMTransition::MAKE(shootState, [this]() { return this->IsAimingAtTarget(); });
    aimState->AddTransition(aimToShootTransition);

    //SHOOT STATE TRANSITIONS:
    UHSMTransition* shootToReloadTransition = UHSMTransition::MAKE(reloadState, [this]() { return !this->HasAmmo(); });
    shootState->AddTransition(shootToReloadTransition);

    UHSMTransition* shootToAimTransition = UHSMTransition::MAKE(aimState, [this]() { return !this->IsAimingAtTarget(); });
    shootState->AddTransition(shootToAimTransition);

    //RELOAD STATE TRANSITIONS:
    UHSMTransition* reloadToShootTransition = UHSMTransition::MAKE(shootState, [this]() { return this->HasAmmo(); });
    reloadState->AddTransition(reloadToShootTransition);

    //INVESTIGATE STATE TRANSITIONS:
    UHSMTransition* investigateToPatrolTransition = UHSMTransition::MAKE(patrolState, [this]() { return !this->investigating; });
    investigateState->AddTransition(investigateToPatrolTransition);

    UHSMTransition* investigateToAttackTransition = UHSMTransition::MAKE(attackStateMachine, [this]() { return this->CanSeeEnemy(); });
    investigateState->AddTransition(investigateToAttackTransition);

    UHSMTransition* investigateToFindHelpTransition = UHSMTransition::MAKE(findHelpState, [this]() { return this->IsVulnerable(); });
    investigateState->AddTransition(investigateToFindHelpTransition);

    //FLEE STATE TRANSITIONS:
    UHSMTransition* fleeToPatrolTransition = UHSMTransition::MAKE(patrolState, [this]() { return this->HasHighHealth() && !this->CanSeeEnemy(); });
    fleeState->AddTransition(fleeToPatrolTransition);

    UHSMTransition* fleeToAttackTransition = UHSMTransition::MAKE(attackStateMachine, [this]() { return this->HasHighHealth() && this->CanSeeEnemy(); });
    fleeState->AddTransition(fleeToAttackTransition);

    //FINDING HELP STATE TRANSITIONS:
    UHSMTransition* findingHelpToPatrolTransition = UHSMTransition::MAKE(patrolState, [this]() { return this->IsNotVulnerable() && !this->CanSeeEnemy() && !hasUninvestigatedLocation; });
    findHelpState->AddTransition(findingHelpToPatrolTransition);

    UHSMTransition* findingHelpToAttackTransition = UHSMTransition::MAKE(attackStateMachine, [this]() { return this->IsNotVulnerable() && this->CanSeeEnemy(); });
    findHelpState->AddTransition(findingHelpToAttackTransition);

    UHSMTransition* findingHelpToInvestigateTransition = UHSMTransition::MAKE(investigateState, [this]() { return this->IsNotVulnerable() && hasUninvestigatedLocation; });
    findHelpState->AddTransition(findingHelpToInvestigateTransition);



    attackStateMachine->AddState(aimState);
    attackStateMachine->AddState(shootState);
    attackStateMachine->AddState(reloadState);

    fsm->AddState(patrolState);
    fsm->AddState(attackStateMachine);
    fsm->AddState(fleeState);
    fsm->AddState(investigateState);
    fsm->AddState(findHelpState);

    fsm->OnEnter();
}

void ASoldierAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (aiConfig->enemies.Num() <= 0)
    {
        TArray<UInfluenceMapPropagator*> allPropagators = propagator->GetInfluenceMapController()->GetPropagators();
        for (UInfluenceMapPropagator* prop : allPropagators)
        {
            if (prop != this->propagator)
            {
                if (this->propagator->GetEnemyTeams().Contains(prop->GetTeam()))
                {
                    aiConfig->enemies.Add(prop->GetOwner());
                }
                else if (this->propagator->GetAlliedTeams().Contains(prop->GetTeam()) || this->propagator->GetTeam() == prop->GetTeam())
                {
                    aiConfig->allies.Add(prop->GetOwner());
                }
            }
        }
    }
    fsm->OnTick();
}

void ASoldierAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    moveCompleted = true;
}

float GetAngleBetweenVectors(FVector vector1, FVector vector2)
{
    vector1.Normalize();
    vector2.Normalize();
    return FMath::RadiansToDegrees(-atan2(vector1.X * vector2.Y - vector1.Y * vector2.X, vector1.X * vector2.X + vector1.Y * vector2.Y));
}

void ASoldierAIController::Die()
{
    dead = true;
    propagator->GetInfluenceMapController()->RemovePropagator(propagator);
    for (int i = 0; i < aiConfig->enemies.Num(); i++)
    {
        if (aiConfig->enemies[i] != nullptr)
        {
            aiConfig->enemies[i]->FindComponentByClass<USoldierAIConfig>()->enemies.Remove(actor);
            if (aiConfig->enemies[i]->FindComponentByClass<USoldierAIConfig>()->target == actor)
            {
                aiConfig->enemies[i]->FindComponentByClass<USoldierAIConfig>()->target = nullptr;
            }
        }
    }
    for (int i = 0; i < aiConfig->allies.Num(); i++)
    {
        if (aiConfig->allies[i] != nullptr)
        {
            aiConfig->allies[i]->FindComponentByClass<USoldierAIConfig>()->allies.Remove(actor);
        }
    }
    actor->Destroy();
}

void ASoldierAIController::AlertToPoint(FVector newInvestigationPoint)
{
    if (!hasUninvestigatedLocation)
    {
        investigationPoint = newInvestigationPoint;
        hasUninvestigatedLocation = true;
    }
}

//STATES LOGIC:

void ASoldierAIController::Patrol()
{
    UGraphNode* currentNode = propagator->GetCurrentNode();
    if (moveCompleted && currentNode != nullptr)
    {
        //Get influence map information:
        std::vector<float> agentInfluences = std::vector<float>(propagator->GetInfluenceMap().size());
        propagator->GetInfluenceMapController()->GetPropagatorInfluenceMap(propagator, agentInfluences);
        std::vector<float> allyInfluences = std::vector<float>(propagator->GetInfluenceMap().size());
        propagator->GetInfluenceMapController()->GetPropagatorAllyInfluenceMap(propagator, allyInfluences);
        TArray<UGraphNode*> nodes = propagator->GetInfluenceMapController()->GetNodes();

        float lowestInfluence = INT_MAX;
        TArray<UGraphNode*> prioritisedNodes;
        TArray<UGraphNode*> validNodes;

        TArray<UGraphNode*> queue;
        TArray<UGraphNode*> visitedNodes;
        queue.Add(currentNode);
        //Visit all node that this agent has influence over using Breadth-First traversal:
        while (queue.Num() > 0)
        {
            currentNode = queue[0];

            TArray<UGraphNode*> neighbours;
            currentNode->GetNeighbours().GenerateKeyArray(neighbours);
            for (UGraphNode* neighbour : neighbours)
            {
                //If the influence at the neighbour is <= the lowest recorded influence, 
                //add it to the list of potential patrol locations:
                if (allyInfluences[neighbour->GetIndex()] <= lowestInfluence)
                {
                    //If value is <, reset the node lists:
                    if (allyInfluences[neighbour->GetIndex()] < lowestInfluence)
                    {
                        lowestInfluence = allyInfluences[neighbour->GetIndex()];
                        prioritisedNodes = TArray<UGraphNode*>();
                        validNodes = TArray<UGraphNode*>();
                    }
                    //Calculate the angle the agent would have to turn to face the node:
                    FVector agentForward = actor->GetActorForwardVector();
                    FVector directionToNeighbour = neighbour->GetCoordinates() - actor->GetActorLocation();
                    directionToNeighbour.Z = 0;
                    agentForward.Z = 0;
                    float angle = GetAngleBetweenVectors(agentForward, directionToNeighbour);
                    //If the node is in the agents FOV, prioritise it:
                    if (angle > -aiConfig->fov && angle < aiConfig->fov)
                    {
                        prioritisedNodes.Add(neighbour);
                    }
                    //Else just add it to list of potential patrol locations:
                    else
                    {
                        validNodes.Add(neighbour);
                    }
                }
                //if the node has not been visited add it to the queue:
                if (!visitedNodes.Contains(neighbour) && !queue.Contains(neighbour) && agentInfluences[currentNode->GetIndex()] >= 0.0f)
                {
                    queue.Add(neighbour);
                }
            }
            visitedNodes.Add(currentNode);
            queue.RemoveAt(0);
        }
        UGraphNode* destNode = nullptr;
        //If there are nodes in agent's fov, 70% chance to move to one:
        if (prioritisedNodes.Num() > 0 && (rand() % 100) < 70)
        {
            destNode = prioritisedNodes[rand() % prioritisedNodes.Num()];
        }
        //30% chance to move to a node out of the agents FOV:
        else if (validNodes.Num() > 0)
        {
            destNode = validNodes[rand() % validNodes.Num()];
        }
        //Move agent to the patrol location:
        if (destNode != nullptr)
        {
            MoveToLocation(destNode->GetCoordinates(), 20.0f, true, true, true, true);
            moveCompleted = false;
        }
    }
}

void ASoldierAIController::Aim()
{
    if (aiConfig->target != nullptr)
    {
        aiConfig->aiming = true;
        //Get agents forward vector, and vector to target:
        FVector gunLocation = aiConfig->gun->GetComponentLocation();
        FVector targetLocation = aiConfig->target->GetTargetLocation();
        targetLocation.Z = gunLocation.Z;
        FVector desiredDirection = targetLocation - gunLocation;
        FVector gunDirection = aiConfig->gun->GetRightVector();
        //Calculate angle between two vectors, and clamp to rotation speed:
        float angle = FMath::Clamp(GetAngleBetweenVectors(desiredDirection, gunDirection), -5.0f, 5.0f);
        //Create and add rotation to agent:
        FRotator NewRotation = FRotator(0, angle, 0);
        FQuat QuatRotation = FQuat(NewRotation);
        actor->AddActorLocalRotation(QuatRotation, false, 0, ETeleportType::None);
    }
}

void ASoldierAIController::Fire()
{
    if (!aiConfig->firing && aiConfig->aimed && aiConfig->target != nullptr)
    {
        aiConfig->FireWeapon();
    }
}

void ASoldierAIController::Reload()
{
    aiConfig->ReloadWeapon();
}

void ASoldierAIController::Flee()
{
    UGraphNode* currentNode = propagator->GetCurrentNode();
    std::vector<float> enemyLOSMap = std::vector<float>(propagator->GetInfluenceMap().size());
    propagator->GetInfluenceMapController()->GetPropagatorEnemyLOSMap(propagator, enemyLOSMap);
    if (currentNode != nullptr && (moveCompleted && enemyLOSMap[currentNode->GetIndex()] > 0.0f) || enemyLOSMap[destIndex] > 0.0f)
    {
        TArray<UGraphNode*> nodes = propagator->GetInfluenceMapController()->GetNodes();
        TArray<UGraphNode*> priorityNodes;
        TArray<UGraphNode*> validNodes;
        for (int i = 0; i < enemyLOSMap.size(); i++)
        {
            //Ignore the node that the agent is currently at:
            if (nodes[i] == currentNode)
            {
                continue;
            }
            //Ignore all nodes that are in view of enemies:
            if (enemyLOSMap[i] > 0.0f)
            {
                continue;
            }
            //Ignore all nodes that are not up against a wall:
            TMap<UGraphNode*, float> neighbours = nodes[i]->GetNeighbours();
            if (neighbours.Num() > 5)
            {
                continue;
            }
            //Calculate if on opposite side of wall from enemy:
            bool onOtherSide = true;
            for (int j = 0; j < aiConfig->enemies.Num(); j++)
            {
                AActor* enemy = aiConfig->enemies[j];
                if (enemy == nullptr)
                {
                    continue;
                }
                FVector dir = enemy->GetActorLocation() - nodes[i]->GetCoordinates();
                dir.Normalize();
                FHitResult hitResult = FHitResult();
                FCollisionQueryParams collisionParams;
                FCollisionResponseParams collisionResponseParams = FCollisionResponseParams(ECollisionResponse::ECR_Block);
                if (!GetWorld()->LineTraceSingleByChannel(hitResult, nodes[i]->GetCoordinates(), nodes[i]->GetCoordinates() + dir * propagator->GetInfluenceMapController()->GetNodeNetwork()->GetResolution() * 1.5f, ECC_GameTraceChannel1, collisionParams, collisionResponseParams))
                {
                    onOtherSide = false;
                    break;
                }
            }
            //If on opposite side of wall from enemy, prioritise the node:
            if (!onOtherSide)
            {
                priorityNodes.Add(nodes[i]);
            }
            else
            {
                validNodes.Add(nodes[i]);
            }
        }
        TArray<UPathNode*> bestPath;
        TArray<UGraphNode*> nodeList = priorityNodes.Num() > 0 ? priorityNodes : validNodes;
        //If there are nodes in cover, move to one:
        if (nodeList.Num() > 0)
        {
            //Find the best path to a point of cover using tactical pathfinding:
            float lowestPathScore = INT_MAX;
            for (UGraphNode* node : nodeList)
            {
                TArray<UPathNode*> path = pathfindingController->RunPathfinding(currentNode->GetIndex(), node->GetIndex(), fleeTacticalInformation);
                float pathLength = pathfindingController->CalculatePathLength(path, fleeTacticalInformation);
                float closestEnemyDistance = INT_MAX;
                for (int i = 0; i < aiConfig->enemies.Num(); i++)
                {
                    UNavigationPath* pathToEnemy = UNavigationSystemV1::GetCurrent(GetWorld())->FindPathToLocationSynchronously(GetWorld(), aiConfig->enemies[i]->GetActorLocation(), node->GetCoordinates());
                    if (pathToEnemy->GetPathLength() < closestEnemyDistance)
                    {
                        closestEnemyDistance = pathToEnemy->GetPathLength();
                    }
                }
                float pathScore = pathLength - closestEnemyDistance;
                if (pathScore < lowestPathScore && path.Num() > 0)
                {
                    bestPath = path;
                    lowestPathScore = pathScore;
                }
            }
        }
        //Else move to the node at which the agent is least vulnerable:
        else
        {
            //Get the propagator's directed vulnerability map and find the node at which he is least vulnerable:
            std::vector<float> directedVulnerabilityMap = std::vector<float>(propagator->GetInfluenceMap().size());
            propagator->GetInfluenceMapController()->GetDirectedVulnerabilityMap(propagator, directedVulnerabilityMap);
            float bestScore = INT_MIN;
            int bestScoreIndex = 0;
            for (int i = 0; i < directedVulnerabilityMap.size(); i++)
            {
                if (directedVulnerabilityMap[i] > bestScore)
                {
                    bestScore = directedVulnerabilityMap[i];
                    bestScoreIndex = i;
                }
            }
            //Find the best path to the least vulnerable node:
            bestPath = pathfindingController->RunPathfinding(currentNode->GetIndex(), bestScoreIndex, fleeTacticalInformation);
        }
        destIndex = bestPath[bestPath.Num() - 1]->node->GetIndex();
        //Give this path to the agent:
        UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent();
        TArray<FVector> locations;
        for (UPathNode* pathNode : bestPath)
        {
            locations.Add(pathNode->node->GetCoordinates());
        }
        AController* controller = Cast<AController>(this);
        FMetaNavMeshPath* MetaNavMeshPath = new FMetaNavMeshPath(locations, *controller);
        TSharedPtr<FMetaNavMeshPath, ESPMode::ThreadSafe> MetaPathPtr(MetaNavMeshPath);
        pathFollowingComponent->RequestMove(FAIMoveRequest(), MetaPathPtr);
        moveCompleted = false;
    }
}

void ASoldierAIController::Investigate()
{
    if (moveCompleted == true)
    {
        FVector aiLocation = actor->GetActorLocation();
        FVector investigationPointLocation = investigationPoint;
        aiLocation.Z = investigationPointLocation.Z;
        FVector desiredDirection = investigationPointLocation - aiLocation;
        FVector forward = actor->GetActorForwardVector();
        //Calculate angle between two vectors, and clamp to rotation speed:
        float angle = FMath::Clamp(GetAngleBetweenVectors(desiredDirection, forward), -5.0f, 5.0f);
        //Create and add rotation to agent:
        FRotator NewRotation = FRotator(0, angle, 0);
        FQuat QuatRotation = FQuat(NewRotation);
        actor->AddActorLocalRotation(QuatRotation, false, 0, ETeleportType::None);

        hasUninvestigatedLocation = false;
        if (timeInvestigating <= 3 && angle <= 5.0f && angle >= -5.0f)
        {
            timeInvestigating += GetWorld()->GetDeltaSeconds();
        }
        else if (timeInvestigating > 3)
        {
            investigating = false;
        }
    }
}

void ASoldierAIController::FindHelp()
{
    if (moveCompleted)
    {
        for (int i = 0; i < aiConfig->allies.Num(); i++)
        {
            AActor* ally = aiConfig->allies[i];
            ASoldierAIController* allyController = (ASoldierAIController*)((APawn*)ally)->GetController();
            if (GetOwner() != nullptr && allyController != nullptr)
            {
                allyController->AlertToPoint(GetOwner()->GetActorLocation());
            }
        }

        std::vector<float> vulnerabilityMap = std::vector<float>(propagator->GetInfluenceMapController()->GetNodes().Num());

        propagator->GetInfluenceMapController()->GetDirectedVulnerabilityMap(propagator, vulnerabilityMap);
        propagator->GetInfluenceMapController()->NormaliseInfluenceMap(vulnerabilityMap);

        int leastVulnerableIndex = 0;
        float leastVulnerableValue = -INT_MAX;

        for (int i = 0; i < vulnerabilityMap.size(); i++)
        {
            if (vulnerabilityMap[i] > leastVulnerableValue)
            {
                leastVulnerableIndex = i;
                leastVulnerableValue = vulnerabilityMap[i];
            }
            else if (vulnerabilityMap[i] == leastVulnerableValue && rand() % 2 == 1)
            {
                leastVulnerableIndex = i;
                leastVulnerableValue = vulnerabilityMap[i];
            }
        }
        TArray<UPathNode*> path = pathfindingController->RunPathfinding(propagator->GetCurrentNode()->GetIndex(), leastVulnerableIndex, fleeTacticalInformation);
        UPathFollowingComponent* pathFollowingComponent = GetPathFollowingComponent();
        //Add all the waypoints from the calculated path:
        TArray<FVector> locations;
        for (UPathNode* pathNode : path)
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

//ENTER / EXIT CALLBACKS:

void ASoldierAIController::OnEnterAttackState()
{
    StopMovement();
    moveCompleted = true;
    hasUninvestigatedLocation = false;
}

void ASoldierAIController::OnExitAttackState()
{
    aiConfig->aiming = false;
}

void ASoldierAIController::OnEnterInvestigateState()
{
    UGraphNode* nodeToInvestigate = propagator->GetInfluenceMapController()->GetClosestNode(investigationPoint);
    TArray<UGraphNode*> possibleNodes = TArray<UGraphNode*>();
    for (TPair<UGraphNode*, float> inViewNode : nodeToInvestigate->GetInViewNodes())
    {
        if (inViewNode.Value < propagator->GetInfluenceRange() && FVector::Dist(inViewNode.Key->GetCoordinates(), actor->GetActorLocation()) < FVector::Dist(inViewNode.Key->GetCoordinates(), investigationPoint))
        {
            possibleNodes.Add(inViewNode.Key);
        }
    }
    if (possibleNodes.Num() > 0)
    {
        MoveToLocation(possibleNodes[rand() % possibleNodes.Num()]->GetCoordinates(), 20.0f, true, true, true, true);
        investigating = true;
        timeInvestigating = 0.0f;
        moveCompleted = false;
    }
    else
    {
        hasUninvestigatedLocation = false;
    }
}

//TRANSITION LOGIC:
void ASoldierAIController::OnAttackToInvestigate()
{
    hasUninvestigatedLocation = true;
    investigationPoint = aiConfig->target->GetActorLocation();
}

//TRANSITION CONDITIONS:

bool ASoldierAIController::IsAimingAtTarget()
{
    if (aiConfig->target != nullptr)
    {
        FVector start = aiConfig->gun->GetComponentLocation();
        FVector end = aiConfig->target->GetTargetLocation();
        FVector dir = end - start;
        FVector gunDirection = aiConfig->gun->GetRightVector();

        dir.Z = 0;
        gunDirection.Z = 0;
        if (GetAngleBetweenVectors(dir, gunDirection) <= 10.0f && GetAngleBetweenVectors(dir, gunDirection) >= -10.0f)
        {
            TArray<FHitResult> hitResults;
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
                else if (hitResult.Actor == aiConfig->target)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool ASoldierAIController::HasAmmo()
{
    return aiConfig->ammo > 0;
}

bool ASoldierAIController::HasLowHealth()
{
    return aiConfig->health <= (aiConfig->maxHealth / 4);
}

bool ASoldierAIController::HasHighHealth()
{
    return aiConfig->health >= (aiConfig->maxHealth * 0.75);
}

bool ASoldierAIController::CanSeeEnemy()
{
    if (aiConfig->enemies.Num() > 0)
    {
        for (int i = 0; i < aiConfig->enemies.Num(); i++)
        {
            AActor* enemy = aiConfig->enemies[i];
            if (enemy == nullptr)
            {
                continue;
            }
            //Calculate angle between agents forward vector and vector to target:
            FVector dir = enemy->GetTargetLocation() - actor->GetTargetLocation();
            FVector actorForward = actor->GetActorForwardVector();
            float angle = GetAngleBetweenVectors(dir, actorForward);
            //Calculate distance to target:
            float dist = FVector::Dist(actor->GetTargetLocation(), enemy->GetTargetLocation());
            //Fire raycast between agent and target:
            TArray<FHitResult> hitResults;
            FCollisionQueryParams collisionParams;
            FCollisionResponseParams collisionResponseParams = FCollisionResponseParams(ECollisionResponse::ECR_Overlap);
            GetWorld()->LineTraceMultiByChannel(hitResults, actor->GetTargetLocation(), enemy->GetTargetLocation(), ECC_WorldDynamic, collisionParams, collisionResponseParams);
            //If the raycast hits anything that is not the target, it is obstructured:
            bool obstructed = false;
            if (angle >= -80 && angle <= 80)
            {
                for (FHitResult hitResult : hitResults)
                {
                    if (hitResult.Actor == actor)
                    {
                        continue;
                    }
                    else if (hitResult.Actor != enemy)
                    {
                        obstructed = true;
                        break;
                    }
                }
                if (!obstructed)
                {
                    aiConfig->target = enemy;
                    return true;
                }
            }
        }
    }
    return false;
}

bool ASoldierAIController::IsVulnerable()
{
    if (propagator->GetCurrentNode() != nullptr)
    {
        std::vector<float> vulnerabilityMap = std::vector<float>(propagator->GetInfluenceMapController()->GetNodes().Num());

        propagator->GetInfluenceMapController()->GetDirectedVulnerabilityMap(propagator, vulnerabilityMap);
        propagator->GetInfluenceMapController()->NormaliseInfluenceMap(vulnerabilityMap);

        return vulnerabilityMap[propagator->GetCurrentNode()->GetIndex()] <= -0.3f;
    }
    return false;
}

bool ASoldierAIController::IsNotVulnerable()
{
    if (propagator->GetCurrentNode() != nullptr)
    {
        std::vector<float> vulnerabilityMap = std::vector<float>(propagator->GetInfluenceMapController()->GetNodes().Num());

        propagator->GetInfluenceMapController()->GetDirectedVulnerabilityMap(propagator, vulnerabilityMap);
        propagator->GetInfluenceMapController()->NormaliseInfluenceMap(vulnerabilityMap);

        return vulnerabilityMap[propagator->GetCurrentNode()->GetIndex()] >= 0.3f;
    }
    return false;
}

