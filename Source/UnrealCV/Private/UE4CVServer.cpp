#include "UnrealCVPrivate.h"
#include "UE4CVServer.h"
#include "PlayerViewMode.h"
#include "ConsoleHelper.h"
#include "ObjectPainter.h"
#include "CaptureManager.h"
#include "CameraHandler.h"
#include "ObjectHandler.h"
#include "PluginHandler.h"
#include "ActionHandler.h"
#include "AliasHandler.h"
#include "ProceduralHandler.h"
#if WITH_EDITOR
#include "UnrealEd.h"
#endif

/** Only available during game play */
APawn* FUE4CVServer::GetPawn()
{
	static UWorld* CurrentWorld = nullptr;
	UWorld* World = GetGameWorld();
	if (CurrentWorld != World)
	{
		CurrentWorld = World;
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			Pawn = PlayerController->GetPawn();
		}
		else
		{
			Pawn = nullptr;
		}
	}
	return Pawn;
}

FUE4CVServer& FUE4CVServer::Get()
{
	static FUE4CVServer Singleton;
	return Singleton;
}

/**
 For UnrealCV server, when a game start:
 1. Start a TCPserver.
 2. Create a command dispatcher
 3. Add command handler to command dispatcher, CameraHandler should be able to access camera
 4. Bind command dispatcher to TCPserver
 5. Bind command dispatcher to UE4 console

 When a new pawn is created.
 1. Update this pawn with GTCaptureComponent
 */

void FUE4CVServer::RegisterCommandHandlers()
{
	// Taken from ctor, because might cause loop-invoke.
	CommandHandlers.Add(new FObjectCommandHandler(CommandDispatcher));
	CommandHandlers.Add(new FCameraCommandHandler(CommandDispatcher));
	CommandHandlers.Add(new FPluginCommandHandler(CommandDispatcher));
	CommandHandlers.Add(new FActionCommandHandler(CommandDispatcher));
	CommandHandlers.Add(new FAliasCommandHandler(CommandDispatcher));
	CommandHandlers.Add(new FProceduralHandler(CommandDispatcher));
	for (FCommandHandler* Handler : CommandHandlers)
	{
		Handler->RegisterCommands();
	}
}

FUE4CVServer::FUE4CVServer()
{
	Config.Load();
	Config.Save(); // Save the configuration back to disk

	// Code defined here should not use FUE4CVServer::Get();
	NetworkManager = NewObject<UNetworkManager>();
	CommandDispatcher = new FCommandDispatcher();
	FConsoleHelper::Get().SetCommandDispatcher(CommandDispatcher);

	NetworkManager->AddToRoot(); // Avoid GC
	NetworkManager->OnReceived().AddRaw(this, &FUE4CVServer::HandleRawMessage);
}

FUE4CVServer::~FUE4CVServer()
{
	// this->NetworkManager->FinishDestroy(); // TODO: Check is this usage correct?
}

UWorld* FUE4CVServer::GetGameWorld()
{
	UWorld* World = nullptr;
	// The correct way to get GameWorld;
#if WITH_EDITOR
	UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine); // TODO: check which macro can determine whether I am in editor
	if (EditorEngine != nullptr)
	{
		World = EditorEngine->PlayWorld;
		if (World != nullptr && World->IsValidLowLevel() && World->IsGameWorld())
		{
			return World;
		}
		else
		{
			UE_LOG(LogUnrealCV, Error, TEXT("Can not get PlayWorld from EditorEngine, %s"),
				World != nullptr ?
					World->IsValidLowLevel() ? 
						World->IsGameWorld() ? TEXT("but all condidtions match") : TEXT("world is not GameWorld")
					: TEXT("world is not valid")
				: TEXT("world is null")
			);
			return nullptr;
		}
	}
#endif

	UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
	if (GameEngine != nullptr)
	{
		World = GameEngine->GetGameWorld();
		if (World != nullptr && World->IsValidLowLevel())
		{
			return World;
		}
		else
		{
			UE_LOG(LogUnrealCV, Error, TEXT("Can not get GameWorld from GameEngine"));
			return nullptr;
		}
	}

	return nullptr;
}


/**
 * Make sure the UE4CVServer is correctly configured.
 */
bool FUE4CVServer::InitWorld()
{
	UWorld *World = GetGameWorld();
	if (World == nullptr)
	{
		return false;
	}
	// Use this to replace BeginPlay()
	static UWorld *CurrentWorld = nullptr;
	if (CurrentWorld != World)
	{
		// Invoke this everytime when the GWorld changes
		// This will happen when the game is stopped and restart in the UE4Editor
		FObjectPainter::Get().Reset(World->GetCurrentLevel());
		//FObjectPainter::Get().PaintColors();

		APawn* NewWorldPawn = GetPawn();
		if (NewWorldPawn)
		{
			FCaptureManager::Get().AttachGTCaptureComponentToCamera(NewWorldPawn);
		}
		
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (Viewport)
		{
			FEngineShowFlags ShowFlags = World->GetGameViewport()->EngineShowFlags;
			FPlayerViewMode::Get().SaveGameDefault(ShowFlags);
		}

		CurrentWorld = World;
	}
	return true;
}

// Each tick of GameThread.
void FUE4CVServer::ProcessPendingRequest()
{
	while (!PendingRequest.IsEmpty())
	{
		if (!InitWorld()) break;

		FRequest Request;
		bool DequeueStatus = PendingRequest.Dequeue(Request);
		check(DequeueStatus);
		int32 RequestId = Request.RequestId;

		FCallbackDelegate CallbackDelegate;
		CallbackDelegate.BindLambda([this, RequestId](FExecStatus ExecStatus)
		{
			UE_LOG(LogUnrealCV, Warning, TEXT("Response: %s"), *ExecStatus.GetMessage());
			FString ReplyRawMessage = FString::Printf(TEXT("%d:%s"), RequestId, *ExecStatus.GetMessage());
			SendClientMessage(ReplyRawMessage);
		});
		CommandDispatcher->ExecAsync(Request.Message, CallbackDelegate);
	}
}

/** Message handler for server */
void FUE4CVServer::HandleRawMessage(const FString& InRawMessage)
{
	UE_LOG(LogUnrealCV, Warning, TEXT("Request: %s"), *InRawMessage);
	// Parse Raw Message
	FString MessageFormat = TEXT("(\\d{1,8}):(.*)");
	FRegexPattern RegexPattern(MessageFormat);
	FRegexMatcher Matcher(RegexPattern, InRawMessage);

	if (Matcher.FindNext())
	{
		// TODO: Handle malform request message
		FString StrRequestId = Matcher.GetCaptureGroup(1);
		FString Message = Matcher.GetCaptureGroup(2);

		uint32 RequestId = FCString::Atoi(*StrRequestId);
		FRequest Request(Message, RequestId);
		this->PendingRequest.Enqueue(Request);
	}
	else
	{
		SendClientMessage(FString::Printf(TEXT("error: Malformat raw message '%s'"), *InRawMessage));
	}
}

void FUE4CVServer::SendClientMessage(FString Message)
{
	// TODO: Do not use game thread to send message.
	NetworkManager->SendMessage(Message);
}
