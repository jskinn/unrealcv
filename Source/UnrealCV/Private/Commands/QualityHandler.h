#pragma once
#include "CommandDispatcher.h"

class FQualityCommandHandler : public FCommandHandler
{
public:
	FQualityCommandHandler(FCommandDispatcher* InCommandDispatcher) : FCommandHandler(InCommandDispatcher)
	{}
	void RegisterCommands();

	/** Get the global mipmap bias applied to all base color textures */
	FExecStatus GetTextureMipmapBias(const TArray<FString>& Args);
	/** Set the global mipmap bias applied to all base color textures */
	FExecStatus SetTextureMipmapBias(const TArray<FString>& Args);
	/** Get whether detail normal maps are enabled */
	FExecStatus GetDetailNormalsEnabled(const TArray<FString>& Args);
	/** Set whether to enable detail normal maps */
	FExecStatus SetDetailNormalsEnabled(const TArray<FString>& Args);
	/** Get whether materials have roughness forced to matte */
	FExecStatus GetRoughnessEnabled(const TArray<FString>& Args);
	/** Set whether to force materials to have matte roughness */
	FExecStatus SetRoughnessEnabled(const TArray<FString>& Args);
	/** Set a global bias to geometry LODs */
	FExecStatus SetGeometryLODBias(const TArray<FString>& Args);
};
