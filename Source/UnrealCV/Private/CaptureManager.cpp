#include "UnrealCVPrivate.h"
#include "CaptureManager.h"

/**
  * Where to put cameras
  * For each camera in the scene, attach SceneCaptureComponent2D to it
  * So that ground truth can be generated, invoked when a new actor is created
  */
void FCaptureManager::AttachGTCaptureComponentToCamera(AActor* Actor)
{
	FServerConfig& Config = FUE4CVServer::Get().Config;
	// TODO: Only support one camera at the beginning
	// TODO: Make this automatic from material loader.

	CaptureComponentList.Empty();

	UGTCaptureComponent* Capturer = UGTCaptureComponent::Create(Actor, Config.SupportedModes);
	CaptureComponentList.Add(Capturer);

	if (Config.EnableRightEye)
	{
		UGTCaptureComponent *RightEye = UGTCaptureComponent::Create(Actor, Config.SupportedModes);
		RightEye->SetRelativeLocation(FVector(0, 40, 0));
		// RightEye->AddLocalOffset(FVector(0, 40, 0)); // TODO: make this configurable
		CaptureComponentList.Add(RightEye);
	}
}

/**
 * @brief Create a new camera in the world
 * @param Location The location of the new camera
 * @param World The world within which to create the camera
 * @return The ID (int32) of the new camera
 */
int32 FCaptureManager::CreateCamera(FVector Location, UWorld* World)
{
	//TODO: Switch to Camera/ACaptureCamera
	ATargetPoint* Point = World->SpawnActor<ATargetPoint>(Location, FRotator(0, 0, 0));
	this->AttachGTCaptureComponentToCamera(Point);
	return CaptureComponentList.Num() - 2;	// AttachGTCaptureComponent creates 2 cameras
}

/**
 * @brief Get the camera with a particular int id
 * @param CameraId Camera ID between 0 and NumCameras()
 * @return 
 */
UGTCaptureComponent* FCaptureManager::GetCamera(int32 CameraId)
{
	if (CameraId < CaptureComponentList.Num() && CameraId >= 0)
	{
		return CaptureComponentList[CameraId];
	}
	else
	{
		return nullptr;
	}
}

int32 FCaptureManager::NumCameras()
{
	return CaptureComponentList.Num();
}
