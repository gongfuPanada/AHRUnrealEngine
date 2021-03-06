// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PrimitiveSceneProxy.h"

class ENGINE_API FNavLinkRenderingProxy : public FPrimitiveSceneProxy
{
private:
	AActor* LinkOwnerActor;
	class INavLinkHostInterface* LinkOwnerHost;

public:
	struct FNavLinkDrawing
	{
		FVector Left;
		FVector Right;
		ENavLinkDirection::Type Direction;
		FColor Color;
		float SnapRadius;
		uint32 SupportedAgentsBits;
	};
	struct FNavLinkSegmentDrawing
	{
		FVector LeftStart, LeftEnd;
		FVector RightStart, RightEnd;
		ENavLinkDirection::Type Direction;
		FColor Color;
		float SnapRadius;
		uint32 SupportedAgentsBits;
	};

private:
	TArray<FNavLinkDrawing> OffMeshPointLinks;
	TArray<FNavLinkSegmentDrawing> OffMeshSegmentLinks;

public:
	/** Initialization constructor. */
	FNavLinkRenderingProxy(const UPrimitiveComponent* InComponent);
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	virtual uint32 GetMemoryFootprint( void ) const override;
	uint32 GetAllocatedSize( void ) const;
	void StorePointLinks(const FTransform& LocalToWorld, const TArray<FNavigationLink>& LinksArray);
	void StoreSegmentLinks(const FTransform& LocalToWorld, const TArray<FNavigationSegmentLink>& LinksArray);

	static void GetLinkMeshes(const TArray<FNavLinkDrawing>& OffMeshPointLinks, const TArray<FNavLinkSegmentDrawing>& OffMeshSegmentLinks, TArray<float>& StepHeights, FMaterialRenderProxy* const MeshColorInstance, int32 ViewIndex, FMeshElementCollector& Collector, uint32 AgentMask);

	/** made static to allow consistent navlinks drawing even if something is drawing links without FNavLinkRenderingProxy */
	static void DrawLinks(FPrimitiveDrawInterface* PDI, TArray<FNavLinkDrawing>& OffMeshPointLinks, TArray<FNavLinkSegmentDrawing>& OffMeshSegmentLinks, TArray<float>& StepHeights, FMaterialRenderProxy* const MeshColorInstance, uint32 AgentMask);
};
