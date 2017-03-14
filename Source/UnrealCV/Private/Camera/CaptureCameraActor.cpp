#include "UnrealCVPrivate.h"
#include "Paths.h"
#include "Modules/ModuleManager.h"
#include "ImageWrapper.h"
#include "Misc/FileHelper.h"
#include "CaptureCameraActor.h"

ACaptureCamera::ACaptureCamera() :
	VisionSensor(nullptr),
	ImageWrapper(nullptr),
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
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
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
		if (ImageWrapper->SetRaw(Frame.Pixels.GetData(), Frame.Pixels.Num() * sizeof(FColor), Frame.BufferSize.X, Frame.BufferSize.Y, ERGBFormat::BGRA, 8))
		{
			FFileHelper::SaveArrayToFile(ImageWrapper->GetCompressed(ImageCompression::Default), *CurrentFilename);
			bSavedImage = true;
		}
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
