#pragma once

#include "CoreMinimal.h"
#include "HSMStateBase.h"
#include "HSMStateMachine.generated.h"

UCLASS()
class STATEMACHINEPROJECT_API UHSMStateMachine : public UHSMStateBase
{
	GENERATED_BODY()
public:
	static UHSMStateMachine* MAKE(std::function<void()> newEnterFunction = nullptr,
								  std::function<void()> newTickFunction = nullptr,
								  std::function<void()> newExitFunction = nullptr,
								  bool resumable = false);


	void AddState(UHSMStateBase* state);
	void SetStartState(UHSMStateBase* state);
	void SetResumable(bool resumable);

	void OnEnter() override;
	void OnTick() override;

private:
	bool isResumable;

	UPROPERTY()
	TArray<UHSMStateBase*> states;

	UPROPERTY()
	UHSMStateBase* initialState;

	UPROPERTY()
	UHSMStateBase* currentState;

	void ChangeCurrentState(UHSMStateBase* state);
};
