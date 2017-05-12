#include "UnrealCVPrivate.h"
#include "ObjectPainter.h"
#include "StaticMeshResources.h"
#include "UE4CVServer.h"
#include "SceneViewport.h"
#include "Version.h"

// Utility function to generate color map
int32 GetChannelValue(uint32 Index)
{
	static int32 Values[256] = { 0 };
	static bool Init = false;
	if (!Init)
	{
		float Step = 256;
		uint32 Iter = 0;
		Values[0] = 0;
		while (Step >= 1)
		{
			for (uint32 Value = Step-1; Value <= 256; Value += Step * 2)
			{
				Iter++;
				Values[Iter] = Value;
			}
			Step /= 2;
		}
		Init = true;
	}
	if (Index >= 0 && Index <= 255)
	{
		return Values[Index];
	}
	else
	{
		UE_LOG(LogUnrealCV, Error, TEXT("Invalid channel index"));
		check(false);
		return -1;
	}
}

void GetColors(int32 MaxVal, bool Fix1, bool Fix2, bool Fix3, TArray<FColor>& ColorMap)
{
	for (int32 I = 0; I <= (Fix1 ? 0 : MaxVal-1); I++)
	{
		for (int32 J = 0; J <= (Fix2 ? 0 : MaxVal-1); J++)
		{
			for (int32 K = 0; K <= (Fix3 ? 0 : MaxVal-1); K++)
			{
				uint8 R = GetChannelValue(Fix1 ? MaxVal : I);
				uint8 G = GetChannelValue(Fix2 ? MaxVal : J);
				uint8 B = GetChannelValue(Fix3 ? MaxVal : K);
				FColor Color(R, G, B, 255);
				ColorMap.Add(Color);
			}
		}
	}
}

// TODO: support more than 1000 objects
FColor GetColorFromColorMap(int32 ObjectIndex)
{
	static TArray<FColor> ColorMap;
	int NumPerChannel = 32;
	if (ColorMap.Num() == 0)
	{
		// 32 ^ 3
		for (int32 MaxChannelIndex = 0; MaxChannelIndex < NumPerChannel; MaxChannelIndex++) // Get color map for 1000 objects
		{
			// GetColors(MaxChannelIndex, false, false, false, ColorMap);
			GetColors(MaxChannelIndex, false, false, true , ColorMap);
			GetColors(MaxChannelIndex, false, true , false, ColorMap);
			GetColors(MaxChannelIndex, false, true , true , ColorMap);
			GetColors(MaxChannelIndex, true , false, false, ColorMap);
			GetColors(MaxChannelIndex, true , false, true , ColorMap);
			GetColors(MaxChannelIndex, true , true , false, ColorMap);
			GetColors(MaxChannelIndex, true , true , true , ColorMap);
		}
	}
	if (ObjectIndex < 0 || ObjectIndex >= pow(NumPerChannel, 3))
	{
		UE_LOG(LogUnrealCV, Error, TEXT("Object index %d is out of the color map boundary [%d, %d]"), ObjectIndex, 0, pow(NumPerChannel, 3));
	}
	return ColorMap[ObjectIndex];
}


FColor GetColorForID(uint32 ObjectId)
{
	// This function matches the post-processing material used, if one changes, this needs to change too.
	float expandedId = (ObjectId + 1.0) / 257.0;
	float R = 131.0 * expandedId;
	float G = 217.0 * expandedId;
	float B = 63 * expandedId;
	// Remove the whole parts to perform modulo
	// This winds up with 257 possible colours, 0-255 mapping to ids, and 256 being the background colour.
	R -= (long) R;
	G -= (long) G;
	B -= (long) B;
	
	// Correct for Gamma encoding
	FLinearColor LinearColor = FLinearColor::FromPow22Color(FColor(255 * R, 255 * G, 255 * B, 255));
	return LinearColor.ToFColor(false);
}

bool ShouldPaintObject(AActor* Actor)
{
	// TODO: Some more conditions on which tags should be painted, at the moment it's at least one non-empty tag.
	for (FName Tag : Actor->Tags)
	{
		if (Tag != "")
		{
			return true;
		}
	}
	return false;
}

/** Check whether an actor can be painted with vertex color */
bool IsPaintable(AActor* Actor)
{
	TArray<UMeshComponent*> PaintableComponents;
	Actor->GetComponents<UMeshComponent>(PaintableComponents);
	return PaintableComponents.Num() == 0;
}

FObjectPainter::FObjectPainter(ULevel* InLevel)
{
	this->Reset(InLevel);
}

FObjectPainter& FObjectPainter::Get()
{
	static FObjectPainter Singleton(nullptr);
	return Singleton;
}

void FObjectPainter::Reset(ULevel* InLevel)
{
	if (InLevel != this->Level)
	{
		this->Level = InLevel;
		if (InLevel != nullptr)
		{
			// We have a new level, we need to rebuild our object map
			this->MappedActors.Empty();
			uint32 IdCounter = 0;
			for (AActor* Actor : Level->Actors)
			{
				if (Actor && IsPaintable(Actor)) 
				{
					if (ShouldPaintObject(Actor))
					{
						this->MappedActors.Insert(Actor, IdCounter);
						this->PaintObject(Actor, IdCounter);
						++IdCounter;
						if (IdCounter >= 256)
						{
							// We've run out of object ids, stop.
							UE_LOG(LogUnrealCV, Error, TEXT("Object Ids exhausted, label fewer objects."));
							break;
						}
					}
					else
					{
						this->UnpaintObject(Actor);
					}
				}
			}
		}
	}
}

AActor* FObjectPainter::GetObject(FString ObjectName)
{
	/** Return the pointer of an object, return NULL if object not found */
	// TODO: Make this more efficient, or stop relying on it. 
	if (this->Level != nullptr)
	{
		for (AActor* Actor : this->Level->Actors)
		{
			if (Actor->GetHumanReadableName() == ObjectName)
			{
				return Actor;
			}
		}
	}
	return nullptr;
}

bool FObjectPainter::PaintObject(AActor* Actor, const uint32 ObjectId)
{
	if (!Actor) return false;

	TArray<UMeshComponent*> PaintableComponents;
	Actor->GetComponents<UMeshComponent>(PaintableComponents);

	for (UMeshComponent* MeshComponent : PaintableComponents)
	{
		MeshComponent->CustomDepthStencilValue = ObjectId;
		MeshComponent->bRenderCustomDepth = true;
		MeshComponent->MarkRenderStateDirty();	// Clear the render state, to display the new colours
	}
	return true;
}

bool FObjectPainter::UnpaintObject(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}
	
	TArray<UMeshComponent*> PaintableComponents;
	Actor->GetComponents<UMeshComponent>(PaintableComponents);

	for (UMeshComponent* MeshComponent : PaintableComponents)
	{
		MeshComponent->CustomDepthStencilValue = 0;
		MeshComponent->bRenderCustomDepth = false;
		MeshComponent->MarkRenderStateDirty();	// Clear the render state, to display the new colours
	}
	return true;
}

FExecStatus FObjectPainter::SetActorColor(FString ObjectName, FColor Color)
{
	return FExecStatus::Error(TEXT("Cannot set actor colours anymore."));
}

FExecStatus FObjectPainter::GetActorColor(FString ObjectName)
{
	FColor ObjectColor(0, 0, 0, 255);
	
	
	for (uint32 ObjectId = 0; ObjectId < this->MappedActors.Num(); ++ObjectId)
	{
		if (this->MappedActors[ObjectId]->GetHumanReadableName() == ObjectName)
		{
			ObjectColor = GetColorForID(ObjectId);
			break;
		}
	}
	
	FString Message = ObjectColor.ToString();
	return FExecStatus::OK(ObjectColor.ToString());
}

// This should be moved to command handler
FExecStatus FObjectPainter::GetObjectList()
{
	FString Message = "";
	for (uint32 ObjectId = 0; ObjectId < this->MappedActors.Num(); ++ObjectId)
	{
		if (ObjectId > 0)
		{
			Message += ", ";
		}
		Message += FString::Printf(TEXT("%d: %s"), ObjectId, *this->MappedActors[ObjectId]->GetHumanReadableName());
	}
	return FExecStatus::OK(Message);
}
