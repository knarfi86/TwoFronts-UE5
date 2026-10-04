#include "TFPlayerState.h"
#include "Net/UnrealNetwork.h"
void ATFPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ATFPlayerState, ChosenFaction); }
