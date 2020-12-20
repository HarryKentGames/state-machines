#include "TargetAISettings.h"

UTargetAISettings::UTargetAISettings()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTargetAISettings::BeginPlay()
{
	Super::BeginPlay();
	
}

void UTargetAISettings::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

