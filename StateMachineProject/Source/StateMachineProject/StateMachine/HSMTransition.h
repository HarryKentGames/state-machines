#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HSMTransition.generated.h"

class UHSMStateBase;

UCLASS()
class STATEMACHINEPROJECT_API UHSMTransition : public UObject
{
	GENERATED_BODY()
public:
	UHSMTransition();
	static UHSMTransition* MAKE(UHSMStateBase* target, std::function<bool()> newCondition = nullptr, std::function<void()> newTransitionFuction = nullptr);
	void SetCondition(std::function<bool()> newCondition);
	bool IsTriggered();
	void OnTransition();
	UHSMStateBase* GetTargetState();
	void SetTargetState(UHSMStateBase* target);
	void SetTransitionFunction(std::function<void()> newTransitionFuction = nullptr);

private:

	UPROPERTY()
	UHSMStateBase* targetState;
	std::function<bool()> condition;
	std::function<void()> transitionFunction;
};
