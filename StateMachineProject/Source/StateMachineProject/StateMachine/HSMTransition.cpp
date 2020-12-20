#include "HSMTransition.h"

UHSMTransition::UHSMTransition()
{
}

UHSMTransition* UHSMTransition::MAKE(UHSMStateBase* target, std::function<bool()> newCondition, std::function<void()> newTransitionFuction)
{
	UHSMTransition* transition = NewObject<UHSMTransition>();
	transition->SetTargetState(target);
	transition->SetCondition(newCondition);
	transition->SetTransitionFunction(newTransitionFuction);
	return transition;
}

void UHSMTransition::SetCondition(std::function<bool()> newCondition)
{
	condition = newCondition;
}

bool UHSMTransition::IsTriggered()
{
	return condition();
}

void UHSMTransition::OnTransition()
{
	if (transitionFunction != nullptr)
	{
		transitionFunction();
	}
}

void UHSMTransition::SetTargetState(UHSMStateBase* target)
{
	targetState = target;
}

UHSMStateBase* UHSMTransition::GetTargetState()
{
	return targetState;
}

void UHSMTransition::SetTransitionFunction(std::function<void()> newTransitionFunction)
{
	transitionFunction = newTransitionFunction;
}
