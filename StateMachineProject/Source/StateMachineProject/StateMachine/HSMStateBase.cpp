#include "HSMStateBase.h"

UHSMStateBase::UHSMStateBase()
{
}

UHSMStateBase::~UHSMStateBase()
{
}

void UHSMStateBase::SetParentMachine(UHSMStateMachine* parent)
{
	parentMachine = parent;
}

void UHSMStateBase::AddTransition(UHSMTransition* transition)
{
	transitions.Add(transition);
}

TArray<UHSMTransition*> UHSMStateBase::GetTransitions()
{
	return transitions;
}

void UHSMStateBase::SetEnterLogic(std::function<void()> newEnterFunction)
{
	enterFunction = newEnterFunction;
}

void UHSMStateBase::SetTickLogic(std::function<void()> newTickFunction)
{
	tickFunction = newTickFunction;
}

void UHSMStateBase::SetExitLogic(std::function<void()> newExitFunction)
{
	exitFunction = newExitFunction;
}

void UHSMStateBase::OnEnter()
{
	if (enterFunction != nullptr)
	{
		enterFunction();
	}
}

void UHSMStateBase::OnTick()
{
	if (tickFunction != nullptr)
	{
		tickFunction();
	}
}

void UHSMStateBase::OnExit()
{
	if (exitFunction != nullptr)
	{
		exitFunction();
	}
}
