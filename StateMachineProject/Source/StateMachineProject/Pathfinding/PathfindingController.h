#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include <chrono>
#include "GraphNodeNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TacticalPathfinder.h"
#include "PathfindingController.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STATEMACHINEPROJECT_API UPathfindingController : public USceneComponent
{
	GENERATED_BODY()

public:
	UPathfindingController();

	static UPathfindingController* FindInstanceInWorld(UWorld* world);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	TArray<UPathNode*> RunPathfinding(int startIndex, int endIndex, TArray<TacticalInformation*> tacticalInformations);
	float CalculatePathLength(TArray<UPathNode*> path, TArray<TacticalInformation*> tacticalInformations);
	void DrawNodes(TArray<UPathNode*> path, FColor color, bool connect);

protected:
	virtual void BeginPlay() override;

private:
	UGraphNodeNetwork* graphController;
	TArray<UGraphNode*> graph;
};
