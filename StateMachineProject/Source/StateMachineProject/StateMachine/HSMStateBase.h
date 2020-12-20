#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HSMTransition.h"
#include "HSMStateBase.generated.h"

class UHSMStateMachine;

UCLASS()
class STATEMACHINEPROJECT_API UHSMStateBase : public UObject
{
	GENERATED_BODY()
public:
	UHSMStateBase();
	~UHSMStateBase();

	void SetParentMachine(UHSMStateMachine* parent);
	void AddTransition(UHSMTransition* transition);
	TArray<UHSMTransition*> GetTransitions();

	void SetEnterLogic(std::function<void()> newEnterFunction);
	void SetTickLogic(std::function<void()> newTickFunction);
	void SetExitLogic(std::function<void()> newExitFunction);

	/**
	 * Called when the state machine enters this state:
	 */
	virtual void OnEnter();
	/**
	 * Called every tick when this is the active state:
	 */
	virtual void OnTick();
	/**
	 * Called when the state machine exits this state:
	 */
	virtual void OnExit();

private:
	UPROPERTY()
	UHSMStateMachine* parentMachine;
	UPROPERTY()
	TArray<UHSMTransition*> transitions;

	std::function<void()> enterFunction;
	std::function<void()> tickFunction;
	std::function<void()> exitFunction;

};
