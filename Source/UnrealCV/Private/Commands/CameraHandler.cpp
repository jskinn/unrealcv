// #include "RealisticRendering.h"
#include "UnrealCVPrivate.h"
#include "CameraHandler.h"
#include "ViewMode.h"
#include "ImageUtils.h"
#include "ImageWrapper.h"
#include "GTCaptureComponent.h"
#include "PlayerViewMode.h"
#include "UE4CVServer.h"
#include "CaptureManager.h"
#include "CineCameraActor.h"
#include "ObjectPainter.h"
#include "ScreenCapture.h"

FString GetDiskFilename(FString Filename)
{
	const FString Dir = FPlatformProcess::BaseDir(); // TODO: Change this to screen capture folder
	// const FString Dir = FPaths::ScreenShotDir();
	FString FullFilename = FPaths::Combine(*Dir, *Filename);

	FString DiskFilename = IFileManager::Get().GetFilenameOnDisk(*FullFilename); // This is important
	return DiskFilename;
}

FString GenerateSeqFilename()
{
	static uint32 NumCaptured = 0;
	NumCaptured++;
	FString Filename = FString::Printf(TEXT("%08d.png"), NumCaptured);
	return Filename;
}

void FCameraCommandHandler::RegisterCommands()
{
	FDispatcherDelegate Cmd;
	FString Help;

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraCount);
	CommandDispatcher->BindCommand("vget /camera/num", Cmd, "Get the number of cameras in the scene");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::CreateCamera);
	CommandDispatcher->BindCommand("vset /camera/create", Cmd, "Create a new camera, the parameters are optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::CreateCamera);
	CommandDispatcher->BindCommand("vset /camera/create [float] [float] [float]", Cmd, "Create a new camera, the parameters are optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/[str]", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/[str] [str]", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetLitViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetLitViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit [str]", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetObjectInstanceMask);
	CommandDispatcher->BindCommand("vget /camera/[uint]/object_mask", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetObjectInstanceMask);
	Help = "Get object mask from camera";
	CommandDispatcher->BindCommand("vget /camera/[uint]/object_mask [str]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetScreenshot);
	Help = "Get snapshot from camera";
	CommandDispatcher->BindCommand("vget /camera/[uint]/screenshot", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraLocation);
	Help = "Get camera location [x, y, z]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/location", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraRotation);
	Help = "Get camera rotation [pitch, yaw, roll]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/rotation", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraLocation);
	Help = "Teleport camera to location [x, y, z]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/location [float] [float] [float]", Cmd, Help);

	/** This is different from SetLocation (which is teleport) */
	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::MoveTo);
	Help = "Move camera to location [x, y, z], will be blocked by objects";
	CommandDispatcher->BindCommand("vset /camera/[uint]/moveto [float] [float] [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraRotation);
	Help = "Set rotation [pitch, yaw, roll] of camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/rotation [float] [float] [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraFOV);
	Help = "Get field of view for camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/fov", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraFOV);
	Help = "Set field of view [float] for camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/fov [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetEnableCameraDOF);
	Help = "Enable or disable depth of field for camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/enable-dof [uint]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetEnableAutofocus);
	Help = "Enable or disable autofocus for camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/autofocus [uint]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraFocusDistance);
	Help = "Get focus distance for camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/focus-distance", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraFocusDistance);
	Help = "Set focus distance [float] for camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/focus-distance [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraFstop);
	Help = "Get fstop for camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/fstop", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraFstop);
	Help = "Set fstop [float] for camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/fstop [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraProjMatrix);
	Help = "Get projection matrix from camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/proj_matrix", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraProjMatrix);
	Help = "Set projection matrix from camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/proj_matrix ", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraObstacleAvoidance);
	Help = "Get a force from nearby objects to camera [id], for obstacle avoidance within radius [radius]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/avoid [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(&FPlayerViewMode::Get(), &FPlayerViewMode::SetMode);
	Help = "Set ViewMode to (lit, normal, depth, object_mask)";
	// CommandDispatcher->BindCommand("vset /viewmode lit", Cmd, Help); // Better to check the correctness at compile time
	// CommandDispatcher->BindCommand("vset /viewmode normal", Cmd, Help); // Better to check the correctness at compile time
	// CommandDispatcher->BindCommand("vset /viewmode depth", Cmd, Help); // Better to check the correctness at compile time
	// CommandDispatcher->BindCommand("vset /viewmode object_mask", Cmd, Help); // Better to check the correctness at compile time
	CommandDispatcher->BindCommand("vset /viewmode [str]", Cmd, Help); // Better to check the correctness at compile time

	Cmd = FDispatcherDelegate::CreateRaw(&FPlayerViewMode::Get(), &FPlayerViewMode::GetMode);
	Help = "Get current ViewMode";
	CommandDispatcher->BindCommand("vget /viewmode", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetActorLocation);
	Help = "Get actor location [x, y, z]";
	CommandDispatcher->BindCommand("vget /actor/location", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetActorRotation);
	Help = "Get actor rotation [pitch, yaw, roll]";
	CommandDispatcher->BindCommand("vget /actor/rotation", Cmd, Help);

	// Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetBuffer);
	// CommandDispatcher->BindCommand("vget /camera/[uint]/buffer", Cmd, "Get buffer of this camera");

	Help = "Return raw binary image data, instead of the image filename";
	Cmd = FDispatcherDelegate::CreateLambda([this](const TArray<FString>& Args) { return this->GetPngBinary(Args, TEXT("lit")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit png", Cmd, Help);
	Cmd = FDispatcherDelegate::CreateLambda([this](const TArray<FString>& Args) { return this->GetPngBinary(Args, TEXT("depth")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/depth png", Cmd, Help);
	Cmd = FDispatcherDelegate::CreateLambda([this](const TArray<FString>& Args) { return this->GetPngBinary(Args, TEXT("normal")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/normal png", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateLambda([this](const TArray<FString>& Args) { return this->GetNpyBinary(Args, TEXT("depth")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/depth npy", Cmd, Help);

	// TODO: object_mask will be handled differently
}

FExecStatus FCameraCommandHandler::GetCameraCount(const TArray<FString>& Args)
{
	return FExecStatus::OK(FString::FromInt(FCaptureManager::Get().NumCameras()));
}

FExecStatus FCameraCommandHandler::CreateCamera(const TArray<FString>& Args)
{
	FVector Location(0,0,0);
	
	if (Args.Num() >= 3)
	{
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		if (!FMath::IsFinite(X))
		{
			X = 0;
		}
		if (!FMath::IsFinite(Y))
		{
			Y = 0;
		}
		if (!FMath::IsFinite(Z))
		{
			Z = 0;
		}
		Location = FVector(X, Y, Z);
	}
	
	int32 CameraId = FCaptureManager::Get().CreateCamera(Location, FUE4CVServer::Get().GetGameWorld());
	return FExecStatus::OK(FString::FromInt(CameraId));
}


FExecStatus FCameraCommandHandler::GetCameraProjMatrix(const TArray<FString>& Args)
{
	// FMatrix& ProjMatrix = FSceneView::ViewProjectionMatrix;
	// this->Character->GetWorld()->GetGameViewport()->Viewport->
	// this->Character
	if (Args.Num() >= 1)
	{
		/*int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		USceneCaptureComponent2D* CaptureComponent = Camera->CaptureComponents.FindRef("Lit");
		CaptureComponent->*/
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraProjMatrix(const TArray<FString>& Args)
{
	// FMatrix& ProjMatrix = FSceneView::ViewProjectionMatrix;
	// this->Character->GetWorld()->GetGameViewport()->Viewport->
	// this->Character
	
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::MoveTo(const TArray<FString>& Args)
{
	/** The API for Character, Pawn and Actor are different */
	if (Args.Num() == 4) // ID, X, Y, Z
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Location = FVector(X, Y, Z);

		// if sweep is true, the object can not move through another object
		// Check invalid location and move back a bit.
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			AActor* CameraActor = Camera->GetOwner();
			if (CameraActor)
			{
				bool Success = CameraActor->SetActorLocation(Location, true, nullptr, ETeleportType::TeleportPhysics);
				if (Success)
				{
					return FExecStatus::OK(TEXT("Moved Camera"));
				}
				else {
					return FExecStatus::Error(TEXT("Failed to move camera"));
				}
			}
			else {
				return FExecStatus::Error(TEXT("Camera owner was NULL"));
			}
		}
		else {
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraLocation(const TArray<FString>& Args)
{
	/** The API for Character, Pawn and Actor are different */
	if (Args.Num() == 4) // ID, X, Y, Z
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Location = FVector(X, Y, Z);

		// if sweep is true, the object can not move through another object
		// Check invalid location and move back a bit.
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			AActor* CameraActor = Camera->GetOwner();
			if (CameraActor)
			{
				bool Success = CameraActor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
				if (Success)
				{
					return FExecStatus::OK(TEXT("Moved Camera"));
				}
				else {
					return FExecStatus::Error(TEXT("Failed to move camera"));
				}
			}
			else {
				return FExecStatus::Error(TEXT("Camera owner was NULL"));
			}
		}
		else {
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 4) // ID, Pitch, Roll, Yaw
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		float Pitch = FCString::Atof(*Args[1]), Yaw = FCString::Atof(*Args[2]), Roll = FCString::Atof(*Args[3]);
		FRotator Rotator = FRotator(Pitch, Yaw, Roll);
		
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			AActor* CameraActor = Camera->GetOwner();
			if (CameraActor)
			{
				// This doesn't work because the camera is tied to the player control rotation on tick.
				//bool Success = CameraActor->SetActorRotation(Rotator, ETeleportType::TeleportPhysics);
				
				bool Success = false;
				APawn* Pawn = Cast<APawn>(CameraActor);
				if (Pawn)
				{
					AController* OwningController = Pawn->GetController();
					if (OwningController && OwningController->IsLocalPlayerController())
					{
						OwningController->SetControlRotation(Rotator);
						Success = true;
					}
				}
				if (!Success)
				{
					Success = CameraActor->SetActorRotation(Rotator, ETeleportType::TeleportPhysics);
				}
				if (Success)
				{
					return FExecStatus::OK(TEXT("Rotated Camera"));
				}
				else {
					return FExecStatus::Error(TEXT("Failed to rotate camera"));
				}
			}
			else {
				return FExecStatus::Error(TEXT("Camera owner was NULL"));
			}
		}
		else {
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetCameraRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		bool bIsMatinee = false;
		FRotator CameraRotation;

		// I think this takes too long with a large scene, cut it out.
		// We're getting the camera actor directly anyway.
		/*
		ACineCameraActor* CineCameraActor = nullptr;
		for (AActor* Actor : GWorld->GetCurrentLevel()->Actors)
		{
			// if (Actor && Actor->IsA(AMatineeActor::StaticClass())) // AMatineeActor is deprecated 
			if (Actor && Actor->IsA(ACineCameraActor::StaticClass()))
			{
				bIsMatinee = true;
				CameraRotation = Actor->GetActorRotation();
				break;
			}
		}*/

		if (!bIsMatinee)
		{
			int32 CameraId = FCString::Atoi(*Args[0]);
			const UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
			if (Camera)
			{
				CameraRotation = Camera->GetOwner()->GetActorRotation();
			}
			else
			{
				return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
			}
		}

		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);

		return FExecStatus::OK(Message);
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraLocation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		bool bIsMatinee = false;

		FVector CameraLocation;
		ACineCameraActor* CineCameraActor = nullptr;
		for (AActor* Actor : this->GetWorld()->GetCurrentLevel()->Actors)
		{
			// if (Actor && Actor->IsA(AMatineeActor::StaticClass())) // AMatineeActor is deprecated 
			if (Actor && Actor->IsA(ACineCameraActor::StaticClass()))
			{
				bIsMatinee = true;
				CameraLocation = Actor->GetActorLocation();
				break;
			}
		}

		if (!bIsMatinee)
		{
			int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
			// APawn* Pawn = FUE4CVServer::Get().GetPawn();
			// CameraLocation = Pawn->GetActorLocation();
			UGTCaptureComponent* CaptureComponent = FCaptureManager::Get().GetCamera(CameraId);
			if (CaptureComponent == nullptr)
			{
				return FExecStatus::Error(FString::Printf(TEXT("Camera %d can not be found."), CameraId));
			}
			CameraLocation = CaptureComponent->GetComponentLocation();
		}

		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraLocation.X, CameraLocation.Y, CameraLocation.Z);

		return FExecStatus::OK(Message);
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetObjectInstanceMask(const TArray<FString>& Args)
{
	if (Args.Num() >= 1) // The first is camera id, the viewmode will nbe added
	{
		// Use command dispatcher is more universal
		FExecStatus ExecStatus = CommandDispatcher->Exec(TEXT("vset /viewmode object_mask"));
		if (ExecStatus != FExecStatusType::OK)
		{
			return ExecStatus;
		}

		TArray<FString> ExtraArgs(Args);
		if (ExtraArgs.Num() == 1)
		{
			ExtraArgs.Add(TEXT("object_mask"));
		}
		else {
			ExtraArgs.Insert(TEXT("object_mask"), 2);
		}
		return GetCameraViewMode(ExtraArgs);
		//ExecStatus = GetScreenshot(Args);
		//return ExecStatus;
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetLitViewMode(const TArray<FString>& Args)
{
	if (Args.Num() >= 1)
	{
		// For this viewmode, The post-effect material needs to be explictly cleared
		FPlayerViewMode::Get().Lit();

		TArray<FString> ExtraArgs(Args);
		if (ExtraArgs.Num() == 1)
		{
			ExtraArgs.Add(TEXT("lit"));
		}
		else {
			ExtraArgs.Insert(TEXT("lit"), 2);
		}
		return GetCameraViewMode(ExtraArgs);
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetCameraViewMode(const TArray<FString>& Args)
{
	if (Args.Num() >= 2) // The first is camera id, the second is ViewMode
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		FString ViewMode = Args[1];

		FString Filename;
		if (Args.Num() >= 3)
		{
			Filename = Args[2];
		}
		else
		{
			Filename = GenerateSeqFilename();
		}

		UGTCaptureComponent* GTCapturer = FCaptureManager::Get().GetCamera(CameraId);
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		FAsyncRecord* AsyncRecord = GTCapturer->Capture(*ViewMode, *Filename); // Due to sandbox implementation of UE4, it is not possible to specify an absolute path directly.
		if (AsyncRecord == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Unrecognized capture mode %s"), *ViewMode));
		}

		// TODO: Check IsPending is problematic.
		FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateLambda([Filename, AsyncRecord]()
		{
			if (AsyncRecord->bIsCompleted)
			{
				AsyncRecord->Destory();
				return FExecStatus::OK(GetDiskFilename(Filename));
			}
			else
			{
				return FExecStatus::Pending();
			}
		});
		FString Message = FString::Printf(TEXT("File will be saved to %s"), *Filename);
		return FExecStatus::AsyncQuery(FPromise(PromiseDelegate));
		// The filename here is just for message, not the fullname on the disk, because we can not know that due to sandbox issue.
	}
	return FExecStatus::InvalidArgument;
}

/** vget /camera/[id]/screenshot */
FExecStatus FCameraCommandHandler::GetScreenshot(const TArray<FString>& Args)
{
	if (Args.Num() >= 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);

		FString Filename;
		if (Args.Num() == 1)
		{
			Filename = GenerateSeqFilename();
		}
		else
		{
			Filename = Args[1];
		}

		// FString FullFilename = GetDiskFilename(Filename);
		if (Filename.ToLower() == TEXT("png"))
		{
			return ScreenCaptureAsyncByQuery(); // return the binary data
		}
		else
		{
			return ScreenCaptureAsyncByQuery(Filename);
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetBuffer(const TArray<FString>& Args)
{
	// Initialize console variables.
	static IConsoleVariable* ICVar1 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFrames"));
	// r.BufferVisualizationDumpFramesAsHDR
	ICVar1->Set(1, ECVF_SetByCode);
	static IConsoleVariable* ICVar2 = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFramesAsHDR"));
	ICVar2->Set(1, ECVF_SetByCode);

	/*
	FHighResScreenshotConfig Config = GetHighResScreenshotConfig();
	Config.bCaptureHDR = true;
	Config.bDumpBufferVisualizationTargets = true;
	*/

	GIsHighResScreenshot = true;

	// Get the viewport
	// FSceneViewport* SceneViewport = this->GetWorld()->GetGameViewport()->GetGameViewport();
	// SceneViewport->TakeHighResScreenShot();
	return FExecStatus::OK();
}

FExecStatus FCameraCommandHandler::GetActorRotation(const TArray<FString>& Args)
{
	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FRotator CameraRotation = Pawn->GetControlRotation();
	FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
	return FExecStatus::OK(Message);
}

FExecStatus FCameraCommandHandler::GetActorLocation(const TArray<FString>& Args)
{
	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FVector CameraLocation = Pawn->GetActorLocation();
	FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraLocation.X, CameraLocation.Y, CameraLocation.Z);
	return FExecStatus::OK(Message);
}

FExecStatus FCameraCommandHandler::GetPngBinary(const TArray<FString>& Args, const FString& ViewMode)
{
	int32 CameraId = FCString::Atoi(*Args[0]);

	UGTCaptureComponent* GTCapturer = FCaptureManager::Get().GetCamera(CameraId);
	if (GTCapturer == nullptr)
	{
		return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
	}

	TArray<uint8> ImgData = GTCapturer->CapturePng(ViewMode);
	return FExecStatus::Binary(ImgData);
}

FExecStatus FCameraCommandHandler::GetNpyBinary(const TArray<FString>& Args, const FString& ViewMode)
{
	int32 CameraId = FCString::Atoi(*Args[0]);

	UGTCaptureComponent* GTCapturer = FCaptureManager::Get().GetCamera(CameraId);
	if (GTCapturer == nullptr)
	{
		return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
	}

	TArray<uint8> ImgData = GTCapturer->CaptureNpy(ViewMode);
	return FExecStatus::Binary(ImgData);
}

FExecStatus FCameraCommandHandler::GetCameraFOV(const TArray<FString>& Args)
{
	if (Args.Num() >= 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		const UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			return FExecStatus::OK(FString::Printf(TEXT("%.3f"), Camera->GetFieldOfView()));
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::SetCameraFOV(const TArray<FString>& Args)
{
	if (Args.Num() >= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			Camera->SetFieldOfView(FCString::Atof(*Args[1]));
			return FExecStatus::OK("Set camera FOV");
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

/** Get and Set camera focus settings */
FExecStatus FCameraCommandHandler::SetEnableCameraDOF(const TArray<FString>& Args)
{
	if (Args.Num() >= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			if (FCString::Atoi(*Args[1]) == 0)
			{
				Camera->SetEnableDepthOfField(false);
				return FExecStatus::OK("Disabled camera Depth of Field");
			}
			else
			{
				Camera->SetEnableDepthOfField(true);
				return FExecStatus::OK("Enabled camera Depth of Field");
			}
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::SetEnableAutofocus(const TArray<FString>& Args)
{
	if (Args.Num() >= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			if (FCString::Atoi(*Args[1]) == 0)
			{
				Camera->SetAutofocus(false);
				return FExecStatus::OK("Disabled camera autofocus");
			}
			else
			{
				Camera->SetAutofocus(true);
				return FExecStatus::OK("Enabled camera autofocus");
			}
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraFocusDistance(const TArray<FString>& Args)
{
	if (Args.Num() >= 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		const UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			return FExecStatus::OK(FString::Printf(TEXT("%.3f"), Camera->GetFocusDistance()));
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::SetCameraFocusDistance(const TArray<FString>& Args)
{
	if (Args.Num() >= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			Camera->SetFocusDistance(FCString::Atof(*Args[1]));
			return FExecStatus::OK("Set camera focus distance");
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraFstop(const TArray<FString>& Args)
{
	if (Args.Num() >= 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		const UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			return FExecStatus::OK(FString::Printf(TEXT("%.3f"), Camera->GetFieldOfView()));
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::SetCameraFstop(const TArray<FString>& Args)
{
	if (Args.Num() >= 2)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			Camera->SetFstop(FCString::Atof(*Args[1]));
			return FExecStatus::OK("Set camera Fstop");
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraObstacleAvoidance(const TArray<FString>& Args)
{
	if (Args.Num() >= 5) // ID, Radius, VelX, VelY, VelZ
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		float Radius = FCString::Atof(*Args[1]);
		float X = FCString::Atof(*Args[2]), Y = FCString::Atof(*Args[3]), Z = FCString::Atof(*Args[4]);
		FVector CameraVelocity = FVector(X, Y, Z);

		UGTCaptureComponent* Camera = FCaptureManager::Get().GetCamera(CameraId);
		if (Camera)
		{
			FVector CameraLocation = Camera->GetComponentLocation();
			FQuat CameraQuat = Camera->GetComponentRotation().Quaternion();
			UWorld* World = FUE4CVServer::Get().GetGameWorld();
			FVector NetForce = FVector(0.0f, 0.0f, 0.0f);
			
			// Step 1: Sphere trace in the velocity direction to check if we're going to hit something
			FCollisionQueryParams TraceParams = FCollisionQueryParams();
			TraceParams.bTraceComplex = true;
			TraceParams.bTraceAsyncScene = true;
			TraceParams.bReturnPhysicalMaterial = false;

			//Re-initialize hit info
			FHitResult HitResult(ForceInit);

			//call GetWorld() from within an actor extending class
			World->SweepSingleByChannel(
				HitResult,			//result
				CameraLocation,		//start
				CameraLocation + 10 * CameraVelocity,	//end
				FQuat(),			//Trace orientation. Irrelevant for spheres
				ECC_Visibility, 	//collision channel
				FCollisionShape::MakeSphere(Radius),
				TraceParams);

			if (HitResult.bBlockingHit)
			{
				FVector AvoidDirection = CameraLocation - HitResult.Location;
				// From v^2 = u^2 + 2as
				// Choose v = 0, s = Distance / 2, u = CameraVelocity
				// Then a = u^2 / Distance, in the avoid direction = (SquareVelocity / SquareDistance) * AvoidDirection
				NetForce += (CameraVelocity.SizeSquared() / AvoidDirection.SizeSquared()) * AvoidDirection;
			}

			// Step 2: Ray trace away from adjacent walls, to give us more space
			//FVector Right = FVector::CrossProduct(CameraVelocity, CameraQuat.GetUpVector());
			//Right.Normalize();
			//FVector Up = FVector::CrossProduct(Right, CameraVelocity);
			//Up.Normalize();
			
			// Return the calculated force
			FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), NetForce.X, NetForce.Y, NetForce.Z);
			return FExecStatus::OK(Message);
		}
		else {
			return FExecStatus::Error(FString::Printf(TEXT("Could not find camera with id %d"), CameraId));
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetPath(const TArray<FString>& Args)
{
	return FExecStatus::Error("TODO: Find a path using the navmesh and return a sequence of waypoints.");
}
