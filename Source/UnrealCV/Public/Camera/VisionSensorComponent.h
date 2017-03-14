#pragma once

#include "FrameCapture.h"
#include "CapturedFrame.h"
#include "../ExecStatus.h"
#include "VisionSensorComponent.generated.h"

DECLARE_DELEGATE_OneParam(FVisionSensorGotImageDelegate, FCapturedFrame&)

/**
* Use USceneCaptureComponent2D to export information from the scene.
* This class needs to be tickable to update the rotation of the USceneCaptureComponent2D
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALCV_API UVisionSensorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UVisionSensorComponent();
	virtual ~UVisionSensorComponent();

	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RequestCaptureFrame();
	void SetDelegate(FVisionSensorGotImageDelegate Delegate);

public:

	// The width of the rendered frames. Default is 1280.
	UPROPERTY(EditAnywhere, Category = "Sensor")
	uint32 Width;

	// The height of the rendered frames. Default is 720.
	UPROPERTY(EditAnywhere, Category = "Sensor")
	uint32 Height;

	// Camera field of view (in degrees).
	UPROPERTY(interp, Category = SceneCapture, meta = (DisplayName = "Field of View", UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0"))
	float FOVAngle;

	UPROPERTY(interp, Category = PostProcessVolume, meta = (ShowOnlyInnerProperties))
	struct FPostProcessSettings PostProcessSettings;

	// Range (0.0, 1.0) where 0 indicates no effect, 1 indicates full effect.
	UPROPERTY(interp, Category = PostProcessVolume, BlueprintReadWrite, meta = (UIMin = "0.0", UIMax = "1.0"))
	float PostProcessBlendWeight;

	// The SceneCaptureComponent used as the viewpoint from which to 'see' the scene.
	UPROPERTY()
	USceneCaptureComponent2D* CaptureComponent;

	// The UTextureRenderTarget2D used as the render target onto which the scene capture will be rendered.
	/*UPROPERTY(Category = Private, VisibleDefaultsOnly)
	UTextureRenderTarget2D* RenderTarget;*/

private:

	USceneCaptureComponent2D* CreateSceneCaptureComponent();

	float FrameDelay;
	FFrameCapture FrameCapture;
	FVisionSensorGotImageDelegate GotFrameDelegate; 
};
