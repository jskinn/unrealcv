#pragma once

#include "RandomStream.h"
#include "ProceduralInterface.generated.h"

UINTERFACE(BlueprintType)
class UProceduralInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IProceduralInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="")
	bool Randomize(FRandomStream Random);
};

