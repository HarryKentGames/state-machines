#include "PathfindingController.h"

UPathfindingController::UPathfindingController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UPathfindingController* UPathfindingController::FindInstanceInWorld(UWorld* world)
{
	//Get all the actors in the current world:
	TArray<AActor*> actors = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(world, AActor::StaticClass(), actors);
	//Find the actor that has an influence map controller attached to it:
	AActor** actor = actors.FindByPredicate([](AActor*& item)
	{
		return item->FindComponentByClass<UPathfindingController>() != nullptr;
	});
	//Return the influence map controller component:
	return (*actor)->FindComponentByClass<UPathfindingController>();
}

void UPathfindingController::BeginPlay()
{
	Super::BeginPlay();
	graphController = this->GetOwner()->FindComponentByClass<UGraphNodeNetwork>();
	graph = graphController->CreateNetwork();
}

TArray<UPathNode*> UPathfindingController::RunPathfinding(int startIndex, int endIndex, TArray<TacticalInformation*> tacticalInformations)
{
	if (startIndex >= 0 && startIndex < graph.Num() && endIndex >= 0 && endIndex < graph.Num())
	{
		TArray<const UGraphNode*> visitedNodes = TArray<const UGraphNode*>();
		TArray<UPathNode*> path = UTacticalPathfinder::FindTacticalPath(graph, graph[startIndex], graph[endIndex], new EuclideanDistance(graph[endIndex]), tacticalInformations, visitedNodes);

		return path;
	}
	return TArray<UPathNode*>();
}

float UPathfindingController::CalculatePathLength(TArray<UPathNode*> path, TArray<TacticalInformation*> tacticalInformations)
{
	float pathLength = 0.0f;
	for (int i = 0; i < path.Num() - 1; i++)
	{
		pathLength += path[i]->node->GetNeighbours()[path[i + 1]->node];
		if (i > 0)
		{
			for (int j = 0; j < tacticalInformations.Num(); j++)
			{
				pathLength += (tacticalInformations[j]->GetQualityAtIndex(path[i]->node->GetIndex()) + tacticalInformations[j]->GetQualityAtIndex(path[i - 1]->node->GetIndex())) / 2;
			}
		}
	}
	return pathLength;
}

void UPathfindingController::DrawNodes(TArray<UPathNode*> path, FColor color, bool connect)
{
	for (int i = 0; i < path.Num(); i++)
	{
		DrawDebugPoint(GetWorld(), path[i]->node->GetCoordinates(), 10, color, false, 0.0f);
		if (connect && i < path.Num() - 1)
		{
			DrawDebugLine(GetWorld(), path[i]->node->GetCoordinates(), path[i + 1]->node->GetCoordinates(), color, false, 0.0f);
		}
	}
}

void UPathfindingController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

