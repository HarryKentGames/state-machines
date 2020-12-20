#include "HSMState.h"

UHSMState* UHSMState::MAKE(std::function<void()> newEnterFunction, std::function<void()> newTickFunction, std::function<void()> newExitFunction)
{
	UHSMState* state = NewObject<UHSMState>();
	state->SetEnterLogic(newEnterFunction);
	state->SetTickLogic(newTickFunction);
	state->SetExitLogic(newExitFunction);
	return state;
}
