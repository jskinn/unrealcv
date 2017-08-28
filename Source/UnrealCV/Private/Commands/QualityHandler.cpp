#include "UnrealCVPrivate.h"
#include "AssetRegistryModule.h"
#include "UE4CVServer.h"
#include "QualityHandler.h"

/**
 * @brief Find a material parameter collection by name
 * @param ParameterCollectionName the name of the collection to find
 * @return The ParameterCollection object, or nullptr if not found.
 */
UMaterialParameterCollection* FindMaterialParameterCollection(FName ParameterCollectionName)
{
	// Find the parameter collection by name from the asset registry.
	UMaterialParameterCollection* ParameterCollection = nullptr;
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AllAssetData;
		const UClass* Class = UMaterialParameterCollection::StaticClass();
		AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), AllAssetData);
		for (FAssetData AssetData : AllAssetData)
		{
			if (AssetData.AssetName == ParameterCollectionName)
			{
				ParameterCollection = Cast<UMaterialParameterCollection>(AssetData.GetAsset());
				if (ParameterCollection != nullptr)
				{
					break;
				}
			}
		}
	}
	return ParameterCollection;
}

void FQualityCommandHandler::RegisterCommands()
{
	FDispatcherDelegate Cmd;
	FString Help;

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::GetTextureMipmapBias);
	Help = "Get global texture mipmap bias to base colors";
	CommandDispatcher->BindCommand(TEXT("vget /quality/texture-mipmap-bias"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::SetTextureMipmapBias);
	Help = "Set global texture mipmap bias to base colors";
	CommandDispatcher->BindCommand(TEXT("vset /quality/texture-mipmap-bias [uint]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::GetDetailNormalsEnabled);
	Help = "Get whether detail normal maps are enabled";
	CommandDispatcher->BindCommand(TEXT("vget /quality/normal-maps-enabled"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::SetDetailNormalsEnabled);
	Help = "Set whether to enable detail normal maps";
	CommandDispatcher->BindCommand(TEXT("vset /quality/normal-maps-enabled [uint]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::GetRoughnessEnabled);
	Help = "Get whether materials have roughness forced to matte";
	CommandDispatcher->BindCommand(TEXT("vget /quality/roughness-enabled"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::SetRoughnessEnabled);
	Help = "Set whether to force materials to have matte roughness";
	CommandDispatcher->BindCommand(TEXT("vset /quality/roughness-enabled [uint]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FQualityCommandHandler::SetGeometryLODBias);
	Help = "Set a global bias to geometry LODS";
	CommandDispatcher->BindCommand(TEXT("vset /quality/geometry-decimation [uint]"), Cmd, Help);
}

FExecStatus FQualityCommandHandler::GetTextureMipmapBias(const TArray<FString>& Args)
{
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	UMaterialParameterCollection* Collection = FindMaterialParameterCollection(TEXT("VisualRealismParameters"));
	if (Collection)
	{
		float Result;
		UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
		if (Instance->GetScalarParameterValue(TEXT("BaseMipMapBias"), Result))
		{
			return FExecStatus::OK(FString::Printf(TEXT("%f"), Result));
		}
		else
		{
			return FExecStatus::Error("Could not find parameter BaseMipMapBias");
		}
	}
	return FExecStatus::Error("Could not find MaterialParameterCollection");
}

FExecStatus FQualityCommandHandler::SetTextureMipmapBias(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 Bias = FCString::Atoi(*Args[0]);
		UWorld* World = FUE4CVServer::Get().GetGameWorld();
		UMaterialParameterCollection* Collection = FindMaterialParameterCollection(TEXT("VisualRealismParameters"));
		if (Collection)
		{
			UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
			if (Instance->SetScalarParameterValue(TEXT("BaseMipMapBias"), Bias))
			{
				return FExecStatus::OK();
			}
			else
			{
				return FExecStatus::Error("Could not find parameter BaseMipMapBias");
			}
		}
		return FExecStatus::Error("Could not find MaterialParameterCollection");
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FQualityCommandHandler::GetDetailNormalsEnabled(const TArray<FString>& Args)
{
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	UMaterialParameterCollection* Collection = FindMaterialParameterCollection(TEXT("VisualRealismParameters"));
	if (Collection)
	{
		float Result;
		UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
		if (Instance->GetScalarParameterValue(TEXT("NormalQuality"), Result))
		{
			return FExecStatus::OK(FString::FromInt(Result > 0 ? 1 : 0));
		}
		else
		{
			return FExecStatus::Error("Could not find parameter NormalQuality");
		}
	}
	return FExecStatus::Error("Could not find MaterialParameterCollection");
}

FExecStatus FQualityCommandHandler::SetDetailNormalsEnabled(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 Enabled = FCString::Atoi(*Args[0]);
		Enabled = Enabled > 0 ? 1 : 0;
		UWorld* World = FUE4CVServer::Get().GetGameWorld();
		UMaterialParameterCollection* Collection = FindMaterialParameterCollection(TEXT("VisualRealismParameters"));
		if (Collection)
		{
			UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
			if (Instance->SetScalarParameterValue(TEXT("NormalQuality"), Enabled))
			{
				return FExecStatus::OK();
			}
			else
			{
				return FExecStatus::Error("Could not find parameter NormalQuality");
			}
		}
		return FExecStatus::Error("Could not find MaterialParameterCollection");
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FQualityCommandHandler::GetRoughnessEnabled(const TArray<FString>& Args)
{
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	UMaterialParameterCollection* Collection = FindMaterialParameterCollection(TEXT("VisualRealismParameters"));
	if (Collection)
	{
		float Result;
		UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
		if (Instance->GetScalarParameterValue(TEXT("RoughnessQuality"), Result))
		{
			return FExecStatus::OK(FString::FromInt(Result > 0 ? 1 : 0));
		}
		else
		{
			return FExecStatus::Error("Could not find parameter RoughnessQuality");
		}
	}
	return FExecStatus::Error("Could not find MaterialParameterCollection");
}

FExecStatus FQualityCommandHandler::SetRoughnessEnabled(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 Enabled = FCString::Atoi(*Args[0]);
		Enabled = Enabled > 0 ? 1 : 0;
		UWorld* World = FUE4CVServer::Get().GetGameWorld();
		UMaterialParameterCollection* Collection = FindMaterialParameterCollection(TEXT("VisualRealismParameters"));
		if (Collection)
		{
			UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
			if (Instance->SetScalarParameterValue(TEXT("RoughnessQuality"), Enabled))
			{
				return FExecStatus::OK();
			}
			else
			{
				return FExecStatus::Error("Could not find parameter RoughnessQuality");
			}
		}
		return FExecStatus::Error("Could not find MaterialParameterCollection");
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FQualityCommandHandler::SetGeometryLODBias(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 LODLevel = FCString::Atoi(*Args[0]);
		UWorld* World = FUE4CVServer::Get().GetGameWorld();
		for (auto ActorIter = TActorIterator<AActor>(World); ActorIter; ++ActorIter)
		{
			TArray<UActorComponent*> Components = ActorIter->GetComponentsByClass(UStaticMeshComponent::StaticClass());
			for (UActorComponent* Component : Components)
			{
				UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Component);
				if (StaticMesh)
				{
					StaticMesh->SetForcedLodModel(LODLevel + 1);
				}
			}
		}
	}
	return FExecStatus::InvalidArgument;
}
