#include "UnrealCVPrivate.h"
#include "Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization.h"
#include "Misc/FileHelper.h"
#include "CaptureCameraActor.h"

ACaptureCamera::ACaptureCamera() :
	VisionSensor(nullptr),
	CurrentFilename(TEXT("")),
	bIsWaitingForImage(false),
	bSavedImage(false)
{
	VisionSensor = CreateDefaultSubobject<UVisionSensorComponent>(TEXT("VisionSensor"));
	VisionSensor->SetupAttachment(RootComponent);
	TWeakObjectPtr<ACaptureCamera> self(this);
	VisionSensor->SetDelegate(FVisionSensorGotImageDelegate::CreateLambda([self](FCapturedFrame& Frame){
		if (self.IsValid()) {
			self.Get()->OnCapturedFrame(Frame);
		}
	}));
	//VisionSensor->SetDelegate(FVisionSensorGotImageDelegate::CreateUObject(this, &ACaptureCamera::OnCapturedFrame));
}

FExecStatus ACaptureCamera::GetCameraViewAsync(const FString& Filename)
{
	const FString Dir = FPlatformProcess::BaseDir(); // TODO: Change this to screen capture folder
	CurrentFilename = FPaths::Combine(*Dir, *Filename);
	bIsWaitingForImage = true;
	bSavedImage = false;
	VisionSensor->RequestCaptureFrame();
	
	//return FExecStatus::AsyncQuery(FPromise(FPromiseDelegate::CreateUObject(this, &ACaptureCamera::GetCaptureStatus)));
	TWeakObjectPtr<ACaptureCamera> self(this);
	return FExecStatus::AsyncQuery(FPromise(FPromiseDelegate::CreateLambda([self](){
		if (self.IsValid())
		{
			return self.Get()->GetCaptureStatus();
		}
		return FExecStatus::Error("Camera is no longer valid");
	})));
}

void ACaptureCamera::OnCapturedFrame(FCapturedFrame& Frame)
{
	if (bIsWaitingForImage)
	{
		TArray<uint8> ImgData = SerializationUtils::Image2Png(Frame.Pixels, Frame.BufferSize.X, Frame.BufferSize.Y);
		FFileHelper::SaveArrayToFile(ImgData, *CurrentFilename);
		bSavedImage = true;
	}
}

FExecStatus ACaptureCamera::GetCaptureStatus()
{
	if (bIsWaitingForImage) 
	{
		return FExecStatus::Pending();
	}
	else if (bSavedImage)
	{
		return FExecStatus::OK(CurrentFilename);
	}
	else
	{
		return FExecStatus::Error(TEXT("Image save failed"));
	}
}
