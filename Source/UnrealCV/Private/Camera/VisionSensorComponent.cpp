#include "UnrealCVPrivate.h"
#include "VisionSensorComponent.h"

//TODO: Put the statistics calculation in. See GTCaptureComponent for an example.
//DECLARE_CYCLE_STAT(TEXT("CaptureFrame"), STAT_SaveExr, STATGROUP_UnrealCV);

UVisionSensorComponent::UVisionSensorComponent() :
	Width(1280),
	Height(720),
	CaptureComponent(nullptr),
	FrameDelay(0),
	FrameCapture(),
	GotFrameDelegate()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

UVisionSensorComponent::~UVisionSensorComponent()
{

}

void UVisionSensorComponent::InitializeComponent()
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// If the component is actually attached to something, set up a new capture component
		CaptureComponent = CreateSceneCaptureComponent();

		UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
		CaptureComponent->TextureTarget = RenderTarget;
		CaptureComponent->TextureTarget->InitAutoFormat(Width, Height);

		FrameCapture = FFrameCapture(RenderTarget);
	}
}

void UVisionSensorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Flush the captured frames
	if (FrameCapture.HasCapturedFrames())
	{
		TArray<FCapturedFrame> CapturedFrames = FrameCapture.GetCapturedFrames();	// This also clears the frames stored in the FrameCapture

		if (GotFrameDelegate.IsBound())
		{
			for (FCapturedFrame& Frame : CapturedFrames)
			{
				GotFrameDelegate.Execute(Frame);
			}
		}
	}
}

void UVisionSensorComponent::RequestCaptureFrame()
{
	FrameCapture.CaptureThisFrame();
}

void UVisionSensorComponent::SetDelegate(FVisionSensorGotImageDelegate Delegate)
{
	GotFrameDelegate = Delegate;
}

/**
 * Create a scene capture component for viewing the scene.
 * This is a function so that I can call it more than once for stereo/lightfield cameras when I get to them.
 */
USceneCaptureComponent2D* UVisionSensorComponent::CreateSceneCaptureComponent()
{
	USceneCaptureComponent2D* SceneCapture = NewObject<USceneCaptureComponent2D>(this->GetOuter());
	SceneCapture->SetupAttachment(this);
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->FOVAngle = FOVAngle;
	SceneCapture->PostProcessSettings = PostProcessSettings;
	SceneCapture->PostProcessBlendWeight = PostProcessBlendWeight;
	return SceneCapture;
}
