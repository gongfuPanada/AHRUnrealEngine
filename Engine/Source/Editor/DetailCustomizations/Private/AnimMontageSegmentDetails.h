// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PreviewScene.h"
#include "AnimGraphDefinitions.h"
#include "Editor/KismetWidgets/Public/SScrubControlPanel.h"
#include "Editor/EditorWidgets/Public/ITransportControl.h"

class FAnimMontageSegmentDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;
};


///////////////////////////////////////////////////
class SAnimationSegmentViewport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SAnimationSegmentViewport )
		: _Skeleton(NULL)
		, _AnimRef(NULL)
		, _IsEditable(true)
	{}
		
		SLATE_ARGUMENT( USkeleton*, Skeleton )
		SLATE_ARGUMENT( UAnimSequenceBase*, AnimRef)
		SLATE_ARGUMENT( TSharedPtr<IPropertyHandle>, AnimRefPropertyHandle )
		SLATE_ARGUMENT( TSharedPtr<IPropertyHandle>, StartTimePropertyHandle )
		SLATE_ARGUMENT( TSharedPtr<IPropertyHandle>, EndTimePropertyHandle );

		SLATE_ATTRIBUTE( bool, IsEditable )	
	SLATE_END_ARGS()
	
public:
	SAnimationSegmentViewport();
	virtual ~SAnimationSegmentViewport();

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	void RefreshViewport();

private:

	void InitSkeleton();

	TSharedPtr<FEditorViewportClient> LevelViewportClient;
	
	TSharedPtr<IPropertyHandle> AnimRefPropertyHandle;
	TSharedPtr<IPropertyHandle> StartTimePropertyHandle;
	TSharedPtr<IPropertyHandle> EndTimePropertyHandle;

	/** Slate viewport for rendering and I/O */
	//TSharedPtr<class FSceneViewport> Viewport;
	TSharedPtr<SViewport> ViewportWidget;

	TSharedPtr<class FSceneViewport> SceneViewport;

	/** Skeleton */
	USkeleton* TargetSkeleton;
	UAnimSequenceBase *AnimRef;

	FPreviewScene PreviewScene;
	class UDebugSkelMeshComponent* PreviewComponent;

	TSharedPtr<STextBlock> Description;

	void CleanupComponent(USceneComponent* Component);
	bool IsVisible() const;

public:

	/** Get Min/Max Input of value **/
	float GetViewMinInput() const;
	float GetViewMaxInput() const;
	
	class UAnimSingleNodeInstance*	GetPreviewInstance() const;

	/** Optional, additional values to draw on the timeline **/
	TArray<float> GetBars() const;
	void OnBarDrag(int32 index, float newPos);
	

};

///////////////////////////////////////////////////
/** This is a slimmed down version of SAnimationScrubPanel and has no ties to persona. It would be best to have these inherit from a more
  *	generic base class so some functionality could be shared.
  */
class SAnimationSegmentScrubPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAnimationSegmentScrubPanel)
		: _LockedSequence()
	{}
		/** If you'd like to lock to one asset for this scrub control, give this**/
		SLATE_ARGUMENT(UAnimSequenceBase*, LockedSequence)
		SLATE_ATTRIBUTE(class UAnimSingleNodeInstance*, PreviewInstance);
		/** View Input range **/
		SLATE_ATTRIBUTE( float, ViewInputMin )
		SLATE_ATTRIBUTE( float, ViewInputMax )
		SLATE_ARGUMENT( bool, bAllowZoom )
		SLATE_ATTRIBUTE( TArray<float>, DraggableBars )
		SLATE_EVENT( FOnScrubBarDrag, OnBarDrag)
	SLATE_END_ARGS()
	
	void Construct( const FArguments& InArgs );
	virtual ~SAnimationSegmentScrubPanel();

	void ReplaceLockedSequence(class UAnimSequenceBase * NewLockedSequence);
protected:

	bool bSliderBeingDragged;	
	FReply OnClick_Forward();
	

	TAttribute<class UAnimSingleNodeInstance*>	PreviewInstance;
	
	void AnimChanged(UAnimationAsset * AnimAsset);

	void OnValueChanged(float NewValue);
	// make sure viewport is freshes
	void OnBeginSliderMovement();
	void OnEndSliderMovement(float NewValue);

	
	EPlaybackMode::Type GetPlaybackMode() const;
	bool IsRealtimeStreamingMode() const;

	float GetScrubValue() const;
	class UAnimSingleNodeInstance* GetPreviewInstance() const;

	TSharedPtr<class SScrubControlPanel> ScrubControlPanel;
	class UAnimSequenceBase* LockedSequence;

	/** Do I need to sync with viewport? **/
	bool DoesSyncViewport() const;
	uint32 GetNumOfFrames() const;
	float GetSequenceLength() const;
};

// FAnimationSegmentViewportClient
class FAnimationSegmentViewportClient : public FEditorViewportClient
{
public:
	FAnimationSegmentViewportClient(FPreviewScene& InPreviewScene);

	// FlEditorViewportClient interface
	virtual FSceneInterface* GetScene() const override;
	virtual FLinearColor GetBackgroundColor() const override { return FLinearColor::Black; }

	// End of FEditorViewportClient

	void UpdateLighting();
};
