#include "HSMState.h"

UHSMState* UHSMState::MAKE(std::function<void()> newEnterFunction, std::function<void()> newTickFunction, std::function<void()> newExitFunction)
{
	UHSMState* state = NewObject<UHSMState>();
	state->SetTickLogic(newTickFunction);
	return state;
}
