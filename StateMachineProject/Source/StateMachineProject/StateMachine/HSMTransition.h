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
	static UHSMTransition* MAKE(UHSMStateBase* target, std::function<bool()> newCondition = nullptr);
	void SetCondition(std::function<bool()> newCondition);
	bool IsTriggered();
	UHSMStateBase* GetTargetState();
	void SetTargetState(UHSMStateBase*);

private:
	UHSMStateBase* targetState;
	std::function<bool()> condition;
};
