// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	URig.cpp: Rig functionality for sharing animations
=============================================================================*/ 

#include "EnginePrivate.h"
#include "Animation/Rig.h"
#include "AnimationRuntime.h"
//@todo should move all this window stuff somewhere else. Persona?

#define LOCTEXT_NAMESPACE "Rig"

FName URig::WorldNodeName(TEXT("World"));

URig::URig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void URig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// verify lots of things
	// make sure it's unique name
	// 
}

// for now these are privates since we don't have good control yet
bool URig::AddNode(FName Name, FName ParentNode, FTransform Transform)
{
	int32 NodeIndex = FindNode(Name);

	if (NodeIndex == INDEX_NONE)
	{
		Nodes.Add(FNode(Name, ParentNode, Transform));

		return true;
	}

	return false;
}

bool URig::DeleteNode(FName Name)
{
	int32 NodeIndex = FindNode(Name);

	if (NodeIndex != INDEX_NONE)
	{
		Nodes.RemoveAt(NodeIndex);
		return true;
	}

	return false;
}

int32 URig::FindNode(const FName& NodeName) const
{
	int32 Index=0;
	for (auto Node: Nodes)
	{
		if (Node.Name == NodeName)
		{
			return Index;
		}

		++Index;
	}

	return INDEX_NONE;
}

const FNode* URig::GetNode(int32 NodeIndex) const
{
	if ( Nodes.IsValidIndex(NodeIndex) )
	{
		return &Nodes[NodeIndex];
	}

	return NULL;
}

FName URig::GetNodeName(int32 NodeIndex) const
{
	if(Nodes.IsValidIndex(NodeIndex))
	{
		return Nodes[NodeIndex].Name;
	}

	return NAME_None;
}

FName URig::GetParentNode(FName& NodeName) const
{
	int32 NodeIndex = FindNode(NodeName);

	if (NodeIndex != INDEX_NONE)
	{
		if (Nodes[NodeIndex].ParentName != NAME_None)
		{
			return Nodes[NodeIndex].ParentName;
		}
	}

	return WorldNodeName;
}

bool URig::AddRigConstraint(FName NodeName, EControlConstraint::Type ConstraintType, EConstraintTransform::Type	TransformType, FName ParentSpace, float Weight /*= 1.f*/)
{
	if (ConstraintType == EControlConstraint::Type::Max)
	{
		// invalid type
		return false;
	}

	// make sure the ParentSpace is valid
	int32 ParentIndex = FindNode(ParentSpace);
	if (ParentIndex == INDEX_NONE)
	{
		// if parent is invalid, set it to World
		ParentSpace = WorldNodeName;
	}

	int32 Index = FindTransformBaseByNodeName(NodeName);

	if (Index == INDEX_NONE)
	{
		FTransformBase NewTransformBase;
		NewTransformBase.Node = NodeName;

		if (ConstraintType < EControlConstraint::Max)
		{
			FRigTransformConstraint NewTransformConstraint;
			NewTransformConstraint.TranformType = TransformType;
			NewTransformConstraint.ParentSpace = ParentSpace;
			NewTransformConstraint.Weight = Weight;

			NewTransformBase.Constraints[ConstraintType].TransformConstraints.Add(NewTransformConstraint);

			TransformBases.Add(NewTransformBase);
		}
	}
	else
	{
		// it exists already, need to make sure we don't have different constraint types
		FTransformBase & TransformBase = TransformBases[Index];

		if (ConstraintType < EControlConstraint::Max)
		{
			FTransformBaseConstraint & ControlConstraint = TransformBase.Constraints[ConstraintType];

			FRigTransformConstraint NewTransformConstraint;
			NewTransformConstraint.TranformType = TransformType;
			NewTransformConstraint.ParentSpace = ParentSpace;
			NewTransformConstraint.Weight = Weight;

			// add new transform constraint
			ControlConstraint.TransformConstraints.Add(NewTransformConstraint);
		}
	}

	return true;
}

int32 URig::GetNodeNum() const
{
	return Nodes.Num();
}

int32 URig::GetTransformBaseNum() const
{
	return TransformBases.Num();
}

const FTransformBase* URig::GetTransformBase(int32 TransformBaseIndex) const
{
	if (TransformBases.IsValidIndex(TransformBaseIndex))
	{
		return &TransformBases[TransformBaseIndex];
	}

	return NULL;
}

const FTransformBase* URig::GetTransformBaseByNodeName(FName NodeName) const
{
	int32 TransformBaseIndex = FindTransformBaseByNodeName(NodeName);

	if (TransformBaseIndex != INDEX_NONE)
	{
		return &TransformBases[TransformBaseIndex];
	}

	return NULL;
}

int32 URig::FindTransformParentNode(int32 NodeIndex, bool bTranslate, int32 Index/*=0*/) const
{
	if (Nodes.IsValidIndex(NodeIndex))
	{
		const FTransformBase* TransformBase = GetTransformBaseByNodeName(Nodes[NodeIndex].Name);

		if(TransformBase)
		{
			FName ParentNodeName = NAME_None;
			if(bTranslate)
			{
				ParentNodeName = TransformBase->Constraints[EControlConstraint::Type::Translation].TransformConstraints[Index].ParentSpace;
			}
			else
			{
				ParentNodeName = TransformBase->Constraints[EControlConstraint::Type::Orientation].TransformConstraints[Index].ParentSpace;
			}

			if(ParentNodeName != NAME_None)
			{
				return  FindNode(ParentNodeName);
			}
		}
	}

	return INDEX_NONE;
}

int32 URig::FindTransformBaseByNodeName(FName NodeName) const
{
	int32 Index=0;

	for (auto TransformBase : TransformBases)
	{
		if (TransformBase.Node == NodeName)
		{
			return Index;
		}

		++Index;
	}

	return INDEX_NONE;
}

void URig::CreateFromSkeleton(const USkeleton* Skeleton, const TMap<int32, int32> & RequiredBones)
{
	// show dialog to choose which bone to add
	if(RequiredBones.Num() >0)
	{
		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		TArray<FTransform> SpaceBaseRefPose;

		FAnimationRuntime::FillUpSpaceBasesRefPose(Skeleton, SpaceBaseRefPose);

		// once selected, add node to the rig
		for (auto It = RequiredBones.CreateConstIterator(); It; ++It)
		{
			int32 BoneIndex = (int32)(It.Key());
			int32 ParentIndex = (int32)(It.Value());

			check (BoneIndex != INDEX_NONE);

			FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
			FName ParentBoneName = WorldNodeName;
			if (ParentIndex != INDEX_NONE)
			{
				ParentBoneName = RefSkeleton.GetBoneName(ParentIndex);
			}

			const FTransform& Transform = SpaceBaseRefPose[BoneIndex];

			AddNode(BoneName, ParentBoneName, Transform);
		}

		// add constraint to parent space and relative transform 
		for(auto It = RequiredBones.CreateConstIterator(); It; ++It)
		{
			int32 BoneIndex = (int32)(It.Key());
			int32 ParentIndex = (int32)(It.Value());

			FName BoneName = RefSkeleton.GetBoneName(BoneIndex);

			if (ParentIndex == INDEX_NONE)
			{
				AddRigConstraint(BoneName, EControlConstraint::Translation, EConstraintTransform::Absoluate, WorldNodeName, 1.f);
				AddRigConstraint(BoneName, EControlConstraint::Orientation, EConstraintTransform::Absoluate, WorldNodeName, 1.f);
			}
			else
			{
				FName ParentBoneName = RefSkeleton.GetBoneName(ParentIndex);
				AddRigConstraint(BoneName, EControlConstraint::Translation, EConstraintTransform::Absoluate, ParentBoneName, 1.f);
				AddRigConstraint(BoneName, EControlConstraint::Orientation, EConstraintTransform::Absoluate, ParentBoneName, 1.f);
			}
		}
	}
}

/*
void URig::CalculateComponentSpace(int32 NodeIndex, const FTransform& LocalTransform, const TArray<FTransform> & TransformBuffer, const FGetParentIndex& DelegateToGetParentIndex, FTransform& OutComponentSpaceTransform) const
{
	int32 ConstraintIndex = FindTransformBaseByNodeName(Nodes[NodeIndex].Name);

	if (ConstraintIndex != INDEX_NONE)
	{
		// now find transform constraint data
		{
			const FTransformBaseConstraint& Constraints = TransformBases[ConstraintIndex].Constraints[EControlConstraint::Type::Orientation];

			// for now we only care for the first one
			const FName ParentName = Constraints.TransformConstraints[0].ParentSpace;
			int32 ParentIndex = INDEX_NONE;

			if(DelegateToGetParentIndex.IsBound())
			{
				ParentIndex = DelegateToGetParentIndex.Execute(ParentName);
			}
			else
			{
				ParentIndex = FindTransformParentNode(NodeIndex, false);
			}

			if(ParentIndex != INDEX_NONE && TransformBuffer.IsValidIndex(ParentIndex))
			{
				FQuat ParentRotation = TransformBuffer[ParentIndex].GetRotation();
				OutComponentSpaceTransform.SetRotation(ParentRotation * LocalTransform.GetRotation());
			}
		}

		// same thing for translation
		{
			const FTransformBaseConstraint& Constraints = TransformBases[ConstraintIndex].Constraints[EControlConstraint::Type::Translation];

			// for now we only care for the first one
			const FName ParentName = Constraints.TransformConstraints[0].ParentSpace;
			int32 ParentIndex = INDEX_NONE;

			if(DelegateToGetParentIndex.IsBound())
			{
				ParentIndex = DelegateToGetParentIndex.Execute(ParentName);
			}
			else
			{
				ParentIndex = FindTransformParentNode(NodeIndex, false);
			}

			if(ParentIndex != INDEX_NONE && TransformBuffer.IsValidIndex(ParentIndex))
			{
				FTransform ParentTransform = TransformBuffer[ParentIndex];
				OutComponentSpaceTransform.SetTranslation(ParentTransform.TransformVector(LocalTransform.GetTranslation()));
			}
		}
		// @todo fix this
		OutComponentSpaceTransform.SetScale3D(LocalTransform.GetScale3D());
	}
}

void URig::CalculateLocalSpace(int32 NodeIndex, const FTransform& ComponentTransform, const TArray<FTransform> & TransformBuffer, const FGetParentIndex& DelegateToGetParentIndex, FTransform& OutLocalSpaceTransform) const
{
	int32 ConstraintIndex = FindTransformBaseByNodeName(Nodes[NodeIndex].Name);

	if(ConstraintIndex != INDEX_NONE)
	{
		// now find transform constraint data
		{
			const FTransformBaseConstraint& Constraints = TransformBases[ConstraintIndex].Constraints[EControlConstraint::Type::Orientation];

			// for now we only care for the first one
			const FName ParentName = Constraints.TransformConstraints[0].ParentSpace;
			int32 ParentIndex = INDEX_NONE;

			if(DelegateToGetParentIndex.IsBound())
			{
				ParentIndex = DelegateToGetParentIndex.Execute(ParentName);
			}
			else
			{
				ParentIndex = FindTransformParentNode(NodeIndex, false);
			}

			if(ParentIndex != INDEX_NONE && TransformBuffer.IsValidIndex(ParentIndex))
			{
				FQuat ParentRotation = TransformBuffer[ParentIndex].GetRotation();
				OutLocalSpaceTransform.SetRotation(ParentRotation.Inverse() * ComponentTransform.GetRotation());
			}
		}

		// same thing for translation
		{
			const FTransformBaseConstraint& Constraints = TransformBases[ConstraintIndex].Constraints[EControlConstraint::Type::Translation];

			// for now we only care for the first one
			const FName ParentName = Constraints.TransformConstraints[0].ParentSpace;
			int32 ParentIndex = INDEX_NONE;

			if(DelegateToGetParentIndex.IsBound())
			{
				ParentIndex = DelegateToGetParentIndex.Execute(ParentName);
			}
			else
			{
				ParentIndex = FindTransformParentNode(NodeIndex, false);
			}

			if(ParentIndex != INDEX_NONE && TransformBuffer.IsValidIndex(ParentIndex))
			{
				FTransform LocalTransform = ComponentTransform.GetRelativeTransform(TransformBuffer[ParentIndex]);
				OutLocalSpaceTransform.SetTranslation(LocalTransform.GetTranslation());
			}
		}

		// @todo fix this
		OutLocalSpaceTransform.SetScale3D(ComponentTransform.GetScale3D());
	}
}*/

void URig::SetAllConstraintsToParents()
{
	for(auto & Control : TransformBases)
	{
		FName ParentNode = GetParentNode(Control.Node);

		Control.Constraints[EControlConstraint::Type::Translation].TransformConstraints[0].ParentSpace = ParentNode;
		Control.Constraints[EControlConstraint::Type::Orientation].TransformConstraints[0].ParentSpace = ParentNode;
	}
}
void URig::SetAllConstraintsToWorld()
{
	for (auto & Control : TransformBases)
	{
		Control.Constraints[EControlConstraint::Type::Translation].TransformConstraints[0].ParentSpace = WorldNodeName;
		Control.Constraints[EControlConstraint::Type::Orientation].TransformConstraints[0].ParentSpace = WorldNodeName;
	}
}
#endif // WITH_EDITOR
#undef LOCTEXT_NAMESPACE 