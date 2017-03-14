#pragma once

#include "SharedPointer.h"
#include "CapturedFrame.h"


/**
 * A Class for capturing rendered frames.
 * One of these will exist per frame source.
 * It is the responsibility of the containing object to call 'CheckForCapturedFrame'.
 *
 * This is based on the code here: https://wiki.unrealengine.com/Render_Target_Lookup
 * And on FFrameGrabber (Runtime/MovieSceneCapture/Public/FrameGrabber.h)
 */
class FFrameCapture
{
public:
	FFrameCapture();
	FFrameCapture(UTextureRenderTarget2D* InRenderTarget);
	
	// Explicit move constructors
	FFrameCapture(FFrameCapture&& In);
	FFrameCapture& operator=(FFrameCapture&& In);
	virtual  ~FFrameCapture();
	
	virtual void CaptureThisFrame();
	virtual bool HasCapturedFrames() const;
	virtual TArray<FCapturedFrame> GetCapturedFrames();

private:
	// Private copy constructors, so that we can only move
	FFrameCapture(const FFrameCapture& In);
	FFrameCapture& operator=(const FFrameCapture& In) { return *this; }

private:

	void FlushCapturedFrame();

	bool bReadPixelsStarted;
	UTextureRenderTarget2D* RenderTarget;
	FRenderCommandFence ReadPixelFence;
	TArray<FColor> Pixels;
	TArray<FCapturedFrame> CapturedFrames;
};


