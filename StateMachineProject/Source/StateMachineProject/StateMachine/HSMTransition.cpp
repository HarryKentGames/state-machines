#include "HSMTransition.h"

UHSMTransition::UHSMTransition()
{
}

UHSMTransition* UHSMTransition::MAKE(UHSMStateBase* target, std::function<bool()> newCondition)
{
	UHSMTransition* transition = NewObject<UHSMTransition>();
	transition->SetTargetState(target);
	transition->SetCondition(newCondition);
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

void UHSMTransition::SetTargetState(UHSMStateBase* target)
{
	targetState = target;
}

UHSMStateBase* UHSMTransition::GetTargetState()
{
	return targetState;
}
