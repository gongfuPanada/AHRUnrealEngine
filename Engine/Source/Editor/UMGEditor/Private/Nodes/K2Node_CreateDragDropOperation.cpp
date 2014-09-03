// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "K2ActionMenuBuilder.h"

#include "K2Node_CreateDragDropOperation.h"

#define LOCTEXT_NAMESPACE "UMG"

UK2Node_CreateDragDropOperation::UK2Node_CreateDragDropOperation(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Creates a new drag drop operation").ToString();
}

FText UK2Node_CreateDragDropOperation::GetBaseNodeTitle() const
{
	return LOCTEXT("CreateWidget_BaseTitle", "Create Drag & Drop Operation");
}

FText UK2Node_CreateDragDropOperation::GetNodeTitleFormat() const
{
	return LOCTEXT("CreateWidget", "Create {ClassName}");
}

UClass* UK2Node_CreateDragDropOperation::GetClassPinBaseClass() const
{
	return UDragDropOperation::StaticClass();
}

void UK2Node_CreateDragDropOperation::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UK2Node_CreateDragDropOperation* TemplateNode = NewObject<UK2Node_CreateDragDropOperation>(GetTransientPackage(), GetClass());

	const FString Category = TEXT("User Interface");
	const FText   MenuDesc = LOCTEXT("CreateDragDropOperationMenuOption", "Create Drag & Drop Operation...");
	const FString Tooltip  = TEXT("Create a new UI drag and drop operation.  Use inside of OnDragDetected.");

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category, MenuDesc, Tooltip);
	NodeAction->NodeTemplate = TemplateNode;
}

void UK2Node_CreateDragDropOperation::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* ClassPin = GetClassPin();
	if ( ClassPin->DefaultObject == NULL )
	{
		ClassPin->DefaultObject = UDragDropOperation::StaticClass();

		UClass* UseSpawnClass = GetClassToSpawn();
		if ( UseSpawnClass != NULL )
		{
			CreatePinsForClass(UseSpawnClass);
		}
	}
}

void UK2Node_CreateDragDropOperation::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if ( CompilerContext.bIsFullCompile )
	{
		static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, CreateDragDropOperation);
		static FString OperationClass_ParamName = FString(TEXT("OperationClass"));

		UK2Node_CreateDragDropOperation* CreateOpNode = this;
		UEdGraphPin* SpawnNodeExec = CreateOpNode->GetExecPin();
		UEdGraphPin* SpawnClassPin = CreateOpNode->GetClassPin();
		UEdGraphPin* SpawnNodeThen = CreateOpNode->GetThenPin();
		UEdGraphPin* SpawnNodeResult = CreateOpNode->GetResultPin();

		UClass* SpawnClass = ( SpawnClassPin != NULL ) ? Cast<UClass>(SpawnClassPin->DefaultObject) : NULL;
		//if ( ( 0 == SpawnClassPin->LinkedTo.Num() ) && ( NULL == SpawnClass ) )
		//{
		//	CompilerContext.MessageLog.Error(*LOCTEXT("CreateWidgetNodeMissingClass_Error", "Create Drag/Drop node @@ must have a class specified.").ToString(), CreateOpNode);
		//	// we break exec links so this is the only error we get, don't want the CreateWidget node being considered and giving 'unexpected node' type warnings
		//	CreateOpNode->BreakAllNodeLinks();
		//	return;
		//}

		//////////////////////////////////////////////////////////////////////////
		// create 'UWidgetBlueprintLibrary::CreateDragDropOperation' call node
		UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(CreateOpNode, SourceGraph);
		CallCreateNode->FunctionReference.SetExternalMember(Create_FunctionName, UWidgetBlueprintLibrary::StaticClass());
		CallCreateNode->AllocateDefaultPins();

		UEdGraphPin* CallCreateExec = CallCreateNode->GetExecPin();
		UEdGraphPin* CallCreateOperationClassPin = CallCreateNode->FindPinChecked(OperationClass_ParamName);
		UEdGraphPin* CallCreateResult = CallCreateNode->GetReturnValuePin();

		// Move 'exec' connection from create widget node to 'UWidgetBlueprintLibrary::CreateDragDropOperation'
		CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec, *CallCreateExec);

		if ( SpawnClassPin->LinkedTo.Num() > 0 )
		{
			// Copy the 'blueprint' connection from the spawn node to 'UWidgetBlueprintLibrary::CreateDragDropOperation'
			CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallCreateOperationClassPin);
		}
		else
		{
			// Copy blueprint literal onto 'UWidgetBlueprintLibrary::CreateDragDropOperation' call 
			CallCreateOperationClassPin->DefaultObject = SpawnClass;
		}

		// Move result connection from spawn node to 'UWidgetBlueprintLibrary::CreateDragDropOperation'
		CallCreateResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
		CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult, *CallCreateResult);

		//////////////////////////////////////////////////////////////////////////
		// create 'set var' nodes

		// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
		UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, CreateOpNode, CallCreateResult, GetClassToSpawn());

		// Move 'then' connection from create widget node to the last 'then'
		CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *LastThen);

		// Break any links to the expanded node
		CreateOpNode->BreakAllNodeLinks();
	}
}

#undef LOCTEXT_NAMESPACE