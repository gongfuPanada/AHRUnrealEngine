// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "RigDetails.h"
#include "Animation/Rig.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE	"RigDetails"

// Table Headers for node list
#define NODE_TABLE_DISPLAYNAME	TEXT("DisplayName")
#define NODE_TABLE_NODENAME		TEXT("NodeName")
#define NODE_TABLE_PARENTNAME	TEXT("ParentName")


TSharedRef<IDetailCustomization> FRigDetails::MakeInstance()
{
	return MakeShareable(new FRigDetails);
}

void FRigDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilderPtr = &DetailBuilder;
	TArray<TWeakObjectPtr<UObject>> Objects;

	DetailBuilder.GetObjectsBeingCustomized(Objects);

	// if more than one, do not customize?
	if (Objects.Num() > 1)
	{
		return;
	}

	ItemBeingEdited = Objects[0];


	IDetailCategoryBuilder& NodeCategory = DetailBuilder.EditCategory("Node");
	IDetailCategoryBuilder& RigControlCategory = DetailBuilder.EditCategory("Constraint Setup");

	RigControlsPropertyHandle = DetailBuilder.GetProperty("RigControls");
	NodesPropertyHandle = DetailBuilder.GetProperty("Nodes");

	// since now we can't really resize the array, we'll just allocate everything here
	// if you reallocate, TSharedPtr<FString> for combo box won't work because
	uint32 NumElement = 0;
	check (FPropertyAccess::Fail != NodesPropertyHandle->AsArray()->GetNumElements(NumElement));

	if ( NumElement > 0 )
	{
		DisplayNameTextBoxes.AddZeroed(NumElement);
	}

	check (FPropertyAccess::Fail != RigControlsPropertyHandle->AsArray()->GetNumElements(NumElement));
	if ( NumElement > 0 )
	{
		ParentSpaceOptionList.AddZeroed(NumElement);
	}

	TSharedRef<FDetailArrayBuilder> NodeArrayBuilder = MakeShareable(new FDetailArrayBuilder(NodesPropertyHandle.ToSharedRef()));
	NodeArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FRigDetails::GenerateNodeArrayElementWidget, &DetailBuilder));

	NodeCategory.AddCustomBuilder( NodeArrayBuilder, false );

	TSharedRef<FDetailArrayBuilder> RigControlArrayBuilder = MakeShareable(new FDetailArrayBuilder(RigControlsPropertyHandle.ToSharedRef()));
	RigControlArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FRigDetails::GenerateRigControlArrayElementWidget, &DetailBuilder));

	// add custom menu
	// -> set all to world
	// -> set all to default parent
	RigControlCategory.AddCustomRow(TEXT(""))
	[
		// two button 1. view 2. save to base pose
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateSP(this, &FRigDetails::OnSetAllToWorld))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("SetAllToWorld_ButtonLabel", "Set All Constraints to World"))
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateSP(this, &FRigDetails::OnSetAllToParent))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Text(LOCTEXT("SetAllToParent_ButtonLabel", "Set All Constraints to Parent"))
		]
	];

	RigControlCategory.AddCustomBuilder( RigControlArrayBuilder, false );
}

void FRigDetails::GenerateNodeArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout)
{
	TSharedRef<IPropertyHandle> DisplayNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, DisplayName)).ToSharedRef();
	TSharedRef<IPropertyHandle> NodeNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, Name)).ToSharedRef();
	TSharedRef<IPropertyHandle> ParentNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FNode, ParentName)).ToSharedRef();

	TSharedPtr<SEditableTextBox> DisplayTextBox;

	// the interface will be node [display name] [parent node]
	// delegate for display name
	FString NodeName, ParentNodeName, DisplayString;
	check (NodeNameProp->GetValueAsDisplayString(NodeName) != FPropertyAccess::Fail);
	check (ParentNameProp->GetValueAsDisplayString(ParentNodeName) != FPropertyAccess::Fail);
	check (DisplayNameProp->GetValueAsDisplayString(DisplayString) != FPropertyAccess::Fail);

	ChildrenBuilder.AddChildContent(TEXT(""))
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(100)
			.Content()
			[
				SNew(STextBlock)
				.Text(NodeName)
				.Font(DetailLayout->GetDetailFontBold())
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(150)
			.Content()
			[
				SNew(STextBlock)
				.Text(FString::Printf(TEXT(" [Parent : %s] "), *ParentNodeName))
				.Font(DetailLayout->GetDetailFont())
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(FString(TEXT("Display Name")))
			.Font(DetailLayout->GetDetailFontBold())
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.FillWidth(1)
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride(250)
			.Content()
			[
				SAssignNew(DisplayTextBox, SEditableTextBox)
				.Text( TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateSP(this, &FRigDetails::GetDisplayName, DisplayNameProp) ) )
				.Font(DetailLayout->GetDetailFont())
				.OnTextChanged(this, &FRigDetails::OnDisplayNameChanged, DisplayNameProp, ArrayIndex)
				.OnTextCommitted(this, &FRigDetails::OnDisplayNameCommitted, DisplayNameProp, ArrayIndex)
				.MinDesiredWidth(200)
			]
		]
	];

	DisplayNameTextBoxes[ArrayIndex] = DisplayTextBox;
};

FText FRigDetails::GetDisplayName(TSharedRef<IPropertyHandle> DisplayNameProp) const
{
	FText DisplayText;
	
	check (FPropertyAccess::Success == DisplayNameProp->GetValueAsDisplayText(DisplayText));

	return DisplayText;
}

void FRigDetails::ValidErrorMessage(const FString & DisplayString, int32 ArrayIndex)
{
	if(DisplayNameTextBoxes.IsValidIndex(ArrayIndex))
	{
		DisplayNameTextBoxes[ArrayIndex]->SetError(TEXT(""));

		if(DisplayString.Len() == 0)
		{
			DisplayNameTextBoxes[ArrayIndex]->SetError(TEXT("Name can't be empty"));
		}
		else
		{
			// verify if this name is unique
			URig * Rig = Cast<URig>(ItemBeingEdited.Get());
			if(Rig)
			{
				FString NewText = DisplayString;
				// make sure that name is unique
				const TArray<FNode> & Nodes = Rig->GetNodes();
				int32 NodeIndex = 0;
				for(auto Node : Nodes)
				{
					if(NodeIndex++ != ArrayIndex && Node.DisplayName == NewText)
					{
						DisplayNameTextBoxes[ArrayIndex]->SetError(TEXT("Name should be unique."));
					}
				}
			}
		}
	}
}

void FRigDetails::OnDisplayNameChanged(const FText& Text, TSharedRef<IPropertyHandle> DisplayNameProp, int32 ArrayIndex)
{
	// still set it since you don't know what they come up with 
	DisplayNameProp->SetValueFromFormattedString(Text.ToString());
	ValidErrorMessage(Text.ToString(), ArrayIndex);
}

void FRigDetails::OnDisplayNameCommitted(const FText& Text, ETextCommit::Type CommitType, TSharedRef<IPropertyHandle> DisplayNameProp, int32 ArrayIndex)
{
	// @todo error check here? I basically need mirror string to avoid the issue
	DisplayNameProp->SetValueFromFormattedString(Text.ToString());
}

void FRigDetails::GenerateRigControlArrayElementWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder, IDetailLayoutBuilder* DetailLayout)
{
	TSharedRef<IPropertyHandle> NodeNameProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigControl, Node)).ToSharedRef();
	TSharedPtr<IPropertyHandleArray> ConstraintsProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigControl, Constraints))->AsArray();

	// translation
	TSharedPtr<IPropertyHandleArray> TransformConstraintsProp_T = ConstraintsProp->GetElement(0)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigControlConstraint, TransformConstraints))->AsArray();
	TSharedRef<IPropertyHandle> ParentNameProp_T = TransformConstraintsProp_T->GetElement(0)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigTransformConstraint, ParentSpace)).ToSharedRef();

	// orientation
	TSharedPtr<IPropertyHandleArray> TransformConstraintsProp_R = ConstraintsProp->GetElement(1)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigControlConstraint, TransformConstraints))->AsArray();
	TSharedRef<IPropertyHandle> ParentNameProp_R = TransformConstraintsProp_R->GetElement(0)->GetChildHandle(GET_MEMBER_NAME_CHECKED(FRigTransformConstraint, ParentSpace)).ToSharedRef();

	// the interface will be node [display name] [parent node]
	// delegate for display name
	FString NodeName, ParentNodeName_T, ParentNodeName_R;
	check(NodeNameProp->GetValueAsDisplayString(NodeName) != FPropertyAccess::Fail);
	check(ParentNameProp_T->GetValueAsDisplayString(ParentNodeName_T) != FPropertyAccess::Fail);
	check(ParentNameProp_R->GetValueAsDisplayString(ParentNodeName_R) != FPropertyAccess::Fail);

	// create string list for picking parent node
	// make sure you don't include itself and find what is curretn selected item
	TArray<TSharedPtr<FString>> & ParentNodeOptions = ParentSpaceOptionList[ArrayIndex];
	ParentNodeOptions.Empty();
	ParentNodeOptions.Add(MakeShareable(new FString(URig::WorldNodeName.ToString())));
	URig * Rig = Cast<URig>(ItemBeingEdited.Get());
	check (Rig);
	const TArray<FNode> & Nodes = Rig->GetNodes();
	int32 NodeIndex = 0, ParentIndex_T=0, ParentIndex_R=0;
	const FNode & CurNode = Nodes[ArrayIndex];
	for(auto Node : Nodes)
	{
		if (NodeIndex != ArrayIndex)
		{
			ParentNodeOptions.Add(MakeShareable(new FString(Node.Name.ToString())));

			if (Node.Name.ToString() == ParentNodeName_T)
			{
				ParentIndex_T = ParentNodeOptions.Num()-1;
			}

			if(Node.Name.ToString() == ParentNodeName_R)
			{
				ParentIndex_R = ParentNodeOptions.Num()-1;
			}
		}

		NodeIndex++;
	}

	ChildrenBuilder.AddChildContent(TEXT(""))
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(100)
			.Content()
			[
				SNew(STextBlock)
				.Text(NodeName)
				.Font(DetailLayout->GetDetailFontBold())
			]
		]

		+SHorizontalBox::Slot()
		.Padding(5, 2)
		.AutoWidth()
		[
			SNew(SVerticalBox)

			+SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.Padding(2)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(100)
					.Content()
					[
						SNew(STextBlock)
						.Text(FString(TEXT("Translation")))
						.Font(DetailLayout->GetDetailFontBold())
					]
				]

				+SHorizontalBox::Slot()
				.Padding(2)
				.FillWidth(1)
				[
					SNew(SBox)
					.WidthOverride(250)
					.Content()
					[
						SNew(STextComboBox)
						.OptionsSource(&ParentNodeOptions)
						.InitiallySelectedItem(ParentNodeOptions[ParentIndex_T])
						.OnSelectionChanged(this, &FRigDetails::OnParentSpaceSelectionChanged, ParentNameProp_T)
					]
				]
			]

			+SVerticalBox::Slot()
			.Padding(2)
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.Padding(2)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(100)
					.Content()
					[
						SNew(STextBlock)
						.Text(FString(TEXT("Orientation")))
						.Font(DetailLayout->GetDetailFontBold())
					]
				]

				+SHorizontalBox::Slot()
				.Padding(2)
				.FillWidth(1)
				[
					SNew(SBox)
					.WidthOverride(250)
					.Content()
					[
						SNew(STextComboBox)
						.OptionsSource(&ParentNodeOptions)
						.InitiallySelectedItem(ParentNodeOptions[ParentIndex_R])
						.OnSelectionChanged(this, &FRigDetails::OnParentSpaceSelectionChanged, ParentNameProp_R)
					]
				]
			]
		]
	];
}

void FRigDetails::OnParentSpaceSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo, TSharedRef<IPropertyHandle> ParentSpacePropertyHandle)
{
	if (SelectedItem.IsValid())
	{
		check (ParentSpacePropertyHandle->SetValueFromFormattedString(*SelectedItem.Get()) != FPropertyAccess::Fail);
	}
}

FReply FRigDetails::OnSetAllToWorld()
{
	URig * Rig = Cast<URig>(ItemBeingEdited.Get());
	check(Rig);

	const FScopedTransaction Transaction(LOCTEXT("SetAllToWorld_Action", "Set All Transform Constraints to World"));
	Rig->Modify();
	Rig->SetAllConstraintsToWorld();
	DetailBuilderPtr->ForceRefreshDetails();

	return FReply::Handled();
}

FReply FRigDetails::OnSetAllToParent()
{
	URig * Rig = Cast<URig>(ItemBeingEdited.Get());
	check(Rig);

	const FScopedTransaction Transaction(LOCTEXT("SetAllToParent_Action", "Set All Transform Constraints to Parent"));
	Rig->Modify();
	Rig->SetAllConstraintsToParents();
	DetailBuilderPtr->ForceRefreshDetails();

	return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE