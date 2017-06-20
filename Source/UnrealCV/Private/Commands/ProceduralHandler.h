#pragma once
#include "CommandDispatcher.h"

class FProceduralHandler : public FCommandHandler
{
public:
	FProceduralHandler(FCommandDispatcher* InCommandDispatcher) : FCommandHandler(InCommandDispatcher) {}
	void RegisterCommands();

	FExecStatus RandomizeObjects(const TArray<FString>& Args);
};

