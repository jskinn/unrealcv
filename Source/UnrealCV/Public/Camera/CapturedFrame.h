#pragma once

#include "IntPoint.h"
#include "Array.h"
#include "Color.h"

/**
 * A struct for storing a captured frame, and it's size. Can only be moved, not copied.
 * Borrowed from FrameGrabber
 */
struct FCapturedFrame
{
	FCapturedFrame(FIntPoint InBufferSize) : BufferSize(InBufferSize) {}

	/** Explicit Move constructor and move assignment */
	FCapturedFrame(FCapturedFrame&& In) : Pixels(MoveTemp(In.Pixels)), BufferSize(In.BufferSize) {}
	FCapturedFrame& operator=(FCapturedFrame&& In) { Pixels = MoveTemp(In.Pixels); BufferSize = In.BufferSize; return *this; }

	/** The color buffer of the captured frame */
	TArray<FColor> Pixels;

	/** The size of the resulting color buffer */
	FIntPoint BufferSize;

private:
	/** private copy constructor and copy assignment */
	FCapturedFrame(const FCapturedFrame& In);
	FCapturedFrame& operator=(const FCapturedFrame& In) { return *this; }
};
