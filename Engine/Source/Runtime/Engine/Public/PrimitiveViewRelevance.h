// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * The different types of relevance a primitive scene proxy can declare towards a particular scene view.
 */
struct FPrimitiveViewRelevance
{
	/** The primitive's static elements are rendered for the view. */
	uint32 bStaticRelevance : 1; 
	/** The primitive's dynamic elements are rendered for the view. */
	uint32 bDynamicRelevance : 1;
	/** The primitive is drawn. */
	uint32 bDrawRelevance : 1;
	/** The primitive is casting a shadow. */
	uint32 bShadowRelevance : 1;
	/** The primitive should render to the custom depth pass. */
	uint32 bRenderCustomDepth : 1;
	/** The primitive should render to the base pass / normal depth / velocity rendering. */
	uint32 bRenderInMainPass : 1;
	/** The primitive is drawn only in the editor and composited onto the scene after post processing */
	uint32 bEditorPrimitiveRelevance : 1;
	/** The primitive is drawn only in the editor and composited onto the scene after post processing using no depth testing */
	uint32 bEditorNoDepthTestPrimitiveRelevance : 1;
	/** The primitive should have GatherSimpleLights called on the proxy when gathering simple lights. */
	uint32 bHasSimpleLights : 1;

	/** The primitive has one or more opaque or masked elements. */
	uint32 bOpaqueRelevance : 1;
	/** The primitive has one or more masked elements. */
	uint32 bMaskedRelevance : 1;
	/** The primitive has one or more distortion elements. */
	uint32 bDistortionRelevance : 1;
	/** The primitive has one or more elements that have SeparateTranslucency. */
	uint32 bSeparateTranslucencyRelevance : 1;
	/** The primitive has one or more elements that have normal translucency. */
	uint32 bNormalTranslucencyRelevance : 1;
	/** The primitive has one or more elements that have the SubsurfaceProfile shading model. */
	uint32 bSubsurfaceProfileRelevance : 1;

	// @RyanTorant
	/** The primitive needs to be voxelized */
	uint32 bNeedsVoxelization : 1;

	// @RyanTorant
	/** The primitive needs to be voxelized every frame */
	uint32 bNeedsEveryFrameVoxelization : 1;

	/** 
	 * Whether this primitive view relevance has been initialized this frame.  
	 * Primitives that have not had ComputeRelevanceForView called on them (because they were culled) will not be initialized,
	 * But we may still need to render them from other views like shadow passes, so this tracks whether we can reuse the cached relevance or not.
	 */
	uint32 bInitializedThisFrame : 1;

	bool HasTranslucency() const 
	{
		return bSeparateTranslucencyRelevance || bNormalTranslucencyRelevance;
	}

	/** Initialization constructor. */
	FPrimitiveViewRelevance():
		bStaticRelevance(false),
		bDynamicRelevance(false),
		bDrawRelevance(false),
		bShadowRelevance(false),
		bRenderCustomDepth(false),
		bRenderInMainPass(true),
		bEditorPrimitiveRelevance(false),
		bEditorNoDepthTestPrimitiveRelevance(false),
		bHasSimpleLights(false),
		bOpaqueRelevance(true),
		bMaskedRelevance(false),
		bDistortionRelevance(false),
		bSeparateTranslucencyRelevance(false),
		bNormalTranslucencyRelevance(false),
		bSubsurfaceProfileRelevance(false),
		bInitializedThisFrame(false),
		bNeedsVoxelization(false),
		bNeedsEveryFrameVoxelization(false)
	{}

	// @RyanTorant
	/** Initialization constructor that takes as an input if it needs voxelization. */
	FPrimitiveViewRelevance(bool needsVoxelization,bool needsEveryFrameVoxelization):
		bStaticRelevance(false),
		bDynamicRelevance(false),
		bDrawRelevance(false),
		bShadowRelevance(false),
		bRenderCustomDepth(false),
		bRenderInMainPass(true),
		bEditorPrimitiveRelevance(false),
		bEditorNoDepthTestPrimitiveRelevance(false),
		bHasSimpleLights(false),
		bOpaqueRelevance(true),
		bMaskedRelevance(false),
		bDistortionRelevance(false),
		bSeparateTranslucencyRelevance(false),
		bNormalTranslucencyRelevance(false),
		bSubsurfaceProfileRelevance(false),
		bInitializedThisFrame(false),
		bNeedsVoxelization(needsVoxelization),
		bNeedsEveryFrameVoxelization(needsEveryFrameVoxelization)
	{}

	/** Bitwise OR operator.  Sets any relevance bits which are present in either FPrimitiveViewRelevance. */
	FPrimitiveViewRelevance& operator|=(const FPrimitiveViewRelevance& B)
	{
		bStaticRelevance |= B.bStaticRelevance != 0;
		bDynamicRelevance |= B.bDynamicRelevance != 0;
		bDrawRelevance |= B.bDrawRelevance != 0;
		bShadowRelevance |= B.bShadowRelevance != 0;
		bOpaqueRelevance |= B.bOpaqueRelevance != 0;
		bMaskedRelevance |= B.bMaskedRelevance != 0;
		bDistortionRelevance |= B.bDistortionRelevance != 0;
		bRenderCustomDepth |= B.bRenderCustomDepth != 0;
		bRenderInMainPass |= B.bRenderInMainPass !=0;
		bEditorPrimitiveRelevance |= B.bEditorPrimitiveRelevance !=0;
		bEditorNoDepthTestPrimitiveRelevance |= B.bEditorNoDepthTestPrimitiveRelevance !=0;
		bHasSimpleLights |= B.bHasSimpleLights != 0;
		bSeparateTranslucencyRelevance |= B.bSeparateTranslucencyRelevance != 0;
		bNormalTranslucencyRelevance |= B.bNormalTranslucencyRelevance != 0;
		bInitializedThisFrame |= B.bInitializedThisFrame;
		bSubsurfaceProfileRelevance |= B.bSubsurfaceProfileRelevance != 0;
		bNeedsVoxelization |= B.bNeedsVoxelization != 0;
		bNeedsEveryFrameVoxelization |= B.bNeedsEveryFrameVoxelization != 0;
		return *this;
	}

	/** Binary bitwise OR operator. */
	friend FPrimitiveViewRelevance operator|(const FPrimitiveViewRelevance& A,const FPrimitiveViewRelevance& B)
	{
		FPrimitiveViewRelevance Result(A);
		Result |= B;
		return Result;
	}
};