#pragma once

#include "CoreMinimal.h"
#include "HSMStateBase.h"
#include "HSMState.generated.h"

UCLASS()
class STATEMACHINEPROJECT_API UHSMState : public UHSMStateBase
{
	GENERATED_BODY()
public:
	static UHSMState* MAKE(std::function<void()> newEnterFunction = nullptr, 
						   std::function<void()> newTickFunction = nullptr, 
						   std::function<void()> newExitFunction = nullptr);
};
