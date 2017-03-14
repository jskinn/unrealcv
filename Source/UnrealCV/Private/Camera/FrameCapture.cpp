#include "UnrealCVPrivate.h"
#include "RenderCommandFence.h"
#include "FrameCapture.h"


struct FReadSurfaceContext
{
	FRenderTarget* SrcRenderTarget;
	TArray<FColor>* OutData;
	FIntRect Rect;
	FReadSurfaceDataFlags Flags;
};

FFrameCapture::FFrameCapture(UTextureRenderTarget2D* InRenderTarget) :
	bReadPixelsStarted(false),
	RenderTarget(InRenderTarget),
	ReadPixelFence(),
	Pixels(),
	CapturedFrames()
{
	if (RenderTarget)
	{
		Pixels.Reserve(RenderTarget->SizeX * RenderTarget->SizeY);
	}
}

FFrameCapture::FFrameCapture() : FFrameCapture(nullptr)
{

}

FFrameCapture::FFrameCapture(FFrameCapture&& In) :
	bReadPixelsStarted(In.bReadPixelsStarted),
	RenderTarget(In.RenderTarget),
	ReadPixelFence(In.ReadPixelFence),
	Pixels(MoveTemp(In.Pixels)),
	CapturedFrames(MoveTemp(In.CapturedFrames))
{
}

FFrameCapture& FFrameCapture::operator=(FFrameCapture&& In)
{
	bReadPixelsStarted = In.bReadPixelsStarted;
	RenderTarget = In.RenderTarget;
	ReadPixelFence = In.ReadPixelFence;
	Pixels = MoveTemp(In.Pixels);
	CapturedFrames = MoveTemp(In.CapturedFrames);
	return *this;
}

FFrameCapture::~FFrameCapture()
{

}

void FFrameCapture::CaptureThisFrame()
{
	if (RenderTarget)
	{
		//borrowed from RenderTarget::ReadPixels()
		FTextureRenderTarget2DResource* RenderResource = (FTextureRenderTarget2DResource*)RenderTarget->Resource;

		// Read the render target surface data back.
		FlushCapturedFrame();
		FReadSurfaceContext ReadSurfaceContext =
		{
			RenderResource,
			&Pixels,
			FIntRect(0, 0, RenderResource->GetSizeXY().X, RenderResource->GetSizeXY().Y),
			FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)
		};

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			ReadSurfaceCommand,
			FReadSurfaceContext, Context, ReadSurfaceContext,
			{
				RHICmdList.ReadSurfaceData(
					Context.SrcRenderTarget->GetRenderTargetTexture(),
					Context.Rect,
					*Context.OutData,
					Context.Flags);
			});

		ReadPixelFence.BeginFence();
		bReadPixelsStarted = true;
	}
}

bool FFrameCapture::HasCapturedFrames() const
{
	return RenderTarget != nullptr && (CapturedFrames.Num() > 0 || (bReadPixelsStarted && ReadPixelFence.IsFenceComplete()));
}

TArray<FCapturedFrame> FFrameCapture::GetCapturedFrames()
{
	FlushCapturedFrame();
	TArray<FCapturedFrame> ReturnFrames;
	Swap(ReturnFrames, CapturedFrames);
	CapturedFrames.Reset();
	return ReturnFrames;
}

void FFrameCapture::FlushCapturedFrame()
{
	if (RenderTarget && bReadPixelsStarted && ReadPixelFence.IsFenceComplete())
	{
		FCapturedFrame CapturedFrame(FIntPoint(RenderTarget->GetSurfaceWidth(), RenderTarget->GetSurfaceHeight()));
		CapturedFrame.Pixels = MoveTemp(Pixels);
		CapturedFrames.Add(MoveTemp(CapturedFrame));
		bReadPixelsStarted = false;
		Pixels.Reset();
	}
}
