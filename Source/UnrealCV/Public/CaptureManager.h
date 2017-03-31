#pragma once
#include "GTCaptureComponent.h"

class FCaptureManager
{
private:
	FCaptureManager() {}
	TArray<UGTCaptureComponent*> CaptureComponentList;

public:
	void AttachGTCaptureComponentToCamera(AActor* Actor);
	static FCaptureManager& Get()
	{
		static FCaptureManager Singleton;
		return Singleton;
	};
	int32 CreateCamera(FVector Location, UWorld* World);
	UGTCaptureComponent* GetCamera(int32 CameraId);
	int32 NumCameras();
};
