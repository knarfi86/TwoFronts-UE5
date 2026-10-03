#include "TFPlayerState.h"
#include "Net/UnrealNetwork.h"
void ATFPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const { Super::GetLifetimeReplicatedProps(Out); DOREPLIFETIME(ATFPlayerState, ChosenFaction); }
