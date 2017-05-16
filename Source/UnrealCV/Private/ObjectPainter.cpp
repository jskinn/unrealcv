#include "UnrealCVPrivate.h"
#include "ObjectPainter.h"
#include "StaticMeshResources.h"
#include "UE4CVServer.h"
#include "SceneViewport.h"
#include "Version.h"


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
	return PaintableComponents.Num() != 0;
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

FColor FObjectPainter::GetColorForID(uint32 ObjectId)
{
	// This function matches the post-processing material used, if one changes, this needs to change too.
	float expandedId = (ObjectId + 1.0) / 257.0;
	float R = 131.0 * expandedId;
	float G = 217.0 * expandedId;
	float B = 63.0 * expandedId;
	// Remove the whole parts to perform modulo
	// This winds up with 257 possible colours, 0-255 mapping to ids, and 256 being the background colour.
	R -= (long) R;
	G -= (long) G;
	B -= (long) B;
	
	// Don't correct for Gamma encoding, this matches the actual output colour to within 1 unit (rounding)
	return FColor(255 * R, 255 * G, 255 * B, 255);
}

/** Get a labelled actor by label id */
AActor* FObjectPainter::GetLabeledActorById(uint32 ObjectId)
{
	if (ObjectId < this->MappedActors.Num())
	{
		return this->MappedActors[ObjectId];
	}
	return nullptr;
}

/** Get the actor based on the label colour */
AActor* FObjectPainter::GetLabeledActorByColor(FColor LabelColor)
{
	int32 bestDist = 255 * 255 * 3;
	uint32 bestId = 0;
	for (uint32 ObjectId = 0; ObjectId < this->MappedActors.Num(); ++ObjectId)
	{
		FColor IdColor = GetColorForID(ObjectId);
		int32 diffR = LabelColor.R - IdColor.R;
		int32 diffG = LabelColor.G - IdColor.G;
		int32 diffB = LabelColor.B - IdColor.B;
		int32 dist = (diffR * diffR) + (diffG * diffG) + (diffB * diffB);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestId = ObjectId;
		}
	}
	return this->MappedActors[bestId];
}

bool FObjectPainter::PaintObject(AActor* Actor, const uint32 ObjectId)
{
	if (!Actor)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("Could not paint object, it was null."));
		return false;
	}
	UE_LOG(LogUnrealCV, Warning, TEXT("Painting Object %s with Id %d"), *Actor->GetHumanReadableName(), ObjectId);

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
