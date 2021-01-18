#include "HSMStateMachine.h"

UHSMStateMachine* UHSMStateMachine::MAKE(std::function<void()> newEnterFunction, std::function<void()> newTickFunction, std::function<void()> newExitFunction, bool resumable)
{
	UHSMStateMachine* fsm = NewObject<UHSMStateMachine>();
	fsm->SetEnterLogic(newEnterFunction);
	fsm->SetTickLogic(newTickFunction);
	fsm->SetExitLogic(newExitFunction);
	fsm->SetResumable(resumable);
	return fsm;
}

void UHSMStateMachine::AddState(UHSMStateBase* state)
{
	state->SetParentMachine(this);
	states.Add(state);
	if (states.Num() == 1 && initialState == nullptr)
	{
		SetStartState(state);
	}
}

void UHSMStateMachine::SetStartState(UHSMStateBase* state)
{
	initialState = state;
}

void UHSMStateMachine::SetResumable(bool resumable)
{
	isResumable = resumable;
}

void UHSMStateMachine::ChangeCurrentState(UHSMStateBase* state)
{
	if (!states.Contains(state))
	{
		return;
	}
	if (currentState != nullptr)
	{
		currentState->OnExit();
	}
	currentState = state;
	currentState->OnEnter();
}

void UHSMStateMachine::OnEnter()
{
	Super::OnEnter();
	if (isResumable && currentState != nullptr)
	{
		currentState->OnEnter();
	}
	ChangeCurrentState(initialState);
}

void UHSMStateMachine::OnTick()
{
	if (currentState == nullptr)
	{
		return;
	}
	for (UHSMTransition* transition : currentState->GetTransitions())
	{
		if (transition->IsTriggered())
		{
			transition->OnTransition();
			ChangeCurrentState(transition->GetTargetState());
			break;
		}
	}
	currentState->OnTick();
}
