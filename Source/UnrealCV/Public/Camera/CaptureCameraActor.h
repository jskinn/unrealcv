#pragma once

#include "IImageWrapper.h"
#include "VisionSensorComponent.h"
#include "ExecStatus.h"
#include "CaptureCameraActor.generated.h"

/**
 * A model of a camera, used by the capture manager to capture frames.
 * 
 * @class ACaptureCamera
 * @author John Skinner
 * @date 14/03/17
 * @brief A model of a camera for capturing frames in UnrealCV
 */
 UCLASS(Blueprintable)
class UNREALCV_API ACaptureCamera : public AActor
{
    GENERATED_BODY()
    
public:

	ACaptureCamera();

	FExecStatus GetCameraViewAsync(const FString& Filename);

	
	void OnCapturedFrame(FCapturedFrame& Frame);
	
	
	FExecStatus GetCaptureStatus();

	UPROPERTY()
    UVisionSensorComponent* VisionSensor;

private:
	FString CurrentFilename;
	bool bIsWaitingForImage;
	bool bSavedImage;
};
