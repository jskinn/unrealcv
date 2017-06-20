#include "UnrealCVPrivate.h"
#include "ProceduralHandler.h"
#include "RandomStream.h"
#include "EngineUtils.h"
#include "UE4CVServer.h"
#include "../Procedural/ProceduralInterface.h"

void FProceduralHandler::RegisterCommands()
{
	FDispatcherDelegate Cmd;
	FString Help;

	Cmd = FDispatcherDelegate::CreateRaw(this, &FProceduralHandler::RandomizeObjects);
	Help = "Re-shuffle randomizable scene elements";
	CommandDispatcher->BindCommand(TEXT("vset /shuffle"), Cmd, Help);
	
	Cmd = FDispatcherDelegate::CreateRaw(this, &FProceduralHandler::RandomizeObjects);
	Help = "Re-shuffle randomizable scene elements with a given random seed.";
	CommandDispatcher->BindCommand(TEXT("vset /shuffle [uint]"), Cmd, Help);
}

FExecStatus FProceduralHandler::RandomizeObjects(const TArray<FString>& Args)
{
	FRandomStream RandomStream;
	if (Args.Num() >= 1)
	{
		RandomStream.Initialize(FCString::Atoi(*Args[0]));
	}
	else
	{
		RandomStream.GenerateNewSeed();
	}

	int32 ShuffleCount = 0;
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
	{
		AActor* Actor = *ActorItr;
		IProceduralInterface* ProceduralActor = Cast<IProceduralInterface>(Actor);
		if (ProceduralActor)
		{
			ProceduralActor->Execute_Randomize(Actor, RandomStream);
			++ShuffleCount;
		}
	}

	return FExecStatus::OK(FString::FromInt(ShuffleCount));
}
