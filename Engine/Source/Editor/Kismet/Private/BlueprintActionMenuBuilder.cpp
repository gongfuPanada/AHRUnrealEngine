// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionMenuItem.h"
#include "BlueprintDragDropMenuItem.h"
#include "BlueprintActionFilter.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#include "K2ActionMenuBuilder.h"		// for FBlueprintPaletteListBuilder/FBlueprintGraphActionListBuilder
#include "EdGraphSchema_K2.h"			// for StaticClass(), bUseLegacyActionMenus, etc.
#include "BlueprintEditor.h"			// for GetMyBlueprintWidget(), and GetIsContextSensitive()
#include "SMyBlueprint.h"				// for SelectionAsVar()
#include "BlueprintEditorUtils.h"		// for FindBlueprintForGraphChecked()
#include "BlueprintEditor.h"			// for GetFocusedGraph()
#include "BlueprintEditorSettings.h"	// for bForceLegacyMenuingSystem

#define LOCTEXT_NAMESPACE "BlueprintActionMenuBuilder"

/*******************************************************************************
 * FBlueprintActionMenuItemFactory
 ******************************************************************************/

class FBlueprintActionMenuItemFactory
{
public:
	/** 
	 * Menu item factory constructor. Sets up the blueprint context, which
	 * is utilized when configuring blueprint menu items' names/tooltips/etc.
	 *
	 * @param  Context	The blueprint context for the menu being built.
	 */
	FBlueprintActionMenuItemFactory(FBlueprintActionContext const& Context);

	/** A root category to perpend every menu item with */
	FText RootCategory;
	/** The menu sort order to set every menu item with */
	int32 MenuGrouping;
	/** Cached context for the blueprint menu being built */
	FBlueprintActionContext const& Context;
	
	/**
	 * Spawns a new FBlueprintActionMenuItem with the node-spawner. Constructs
	 * the menu item's category, name, tooltip, etc.
	 * 
	 * @param  EditorContext	
	 * @param  Action			The node-spawner that the new menu item should wrap.
	 * @return A newly allocated FBlueprintActionMenuItem (which wraps the supplied action).
	 */
	TSharedPtr<FBlueprintActionMenuItem> MakeActionMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo const& ActionInfo);

	/**
	 * Spawns a new FBlueprintDragDropMenuItem with the node-spawner. Constructs
	 * the menu item's category, name, tooltip, etc.
	 * 
	 * @param  SampleAction	One of the (possibly) many node-spawners that this menu item is set to represent.
	 * @return A newly allocated FBlueprintActionMenuItem (which wraps the supplied action).
	 */
	TSharedPtr<FBlueprintDragDropMenuItem> MakeDragDropMenuItem(UBlueprintNodeSpawner const* SampleAction);

	/**
	 * 
	 * 
	 * @param  BoundAction	
	 * @return 
	 */
	TSharedPtr<FBlueprintActionMenuItem> MakeBoundMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo const& ActionInfo);
	
private:
	/**
	 * Utility getter function that retrieves the blueprint context for the menu
	 * items being made.
	 * 
	 * @return The first blueprint out of the cached FBlueprintActionContext.
	 */
	UBlueprint* GetTargetBlueprint() const;

	/**
	 * 
	 * 
	 * @param  EditorContext	
	 * @return 
	 */
	UEdGraph* GetTargetGraph(TWeakPtr<FBlueprintEditor> EditorContext) const;

	/**
	 *
	 *
	 * @param  EditorContext
	 * @param  ActionInfo
	 * @return
	 */
	FBlueprintActionUiSpec GetActionUiSignature(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo const& ActionInfo);
};

//------------------------------------------------------------------------------
FBlueprintActionMenuItemFactory::FBlueprintActionMenuItemFactory(FBlueprintActionContext const& ContextIn)
	: RootCategory(FText::GetEmpty())
	, MenuGrouping(0)
	, Context(ContextIn)
{
}

//------------------------------------------------------------------------------
TSharedPtr<FBlueprintActionMenuItem> FBlueprintActionMenuItemFactory::MakeActionMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo const& ActionInfo)
{
	FBlueprintActionUiSpec UiSignature = GetActionUiSignature(EditorContext, ActionInfo);

	UBlueprintNodeSpawner const* Action = ActionInfo.NodeSpawner;
	FBlueprintActionMenuItem* NewMenuItem = new FBlueprintActionMenuItem(Action, UiSignature);

	NewMenuItem->Grouping = MenuGrouping;
	NewMenuItem->Category = FString::Printf(TEXT("%s|%s"), *RootCategory.ToString(), *NewMenuItem->Category);

	return MakeShareable(NewMenuItem);
}

//------------------------------------------------------------------------------
TSharedPtr<FBlueprintDragDropMenuItem> FBlueprintActionMenuItemFactory::MakeDragDropMenuItem(UBlueprintNodeSpawner const* SampleAction)
{
	// FBlueprintDragDropMenuItem takes care of its own menu MenuDescription, etc.
	FBlueprintDragDropMenuItem* NewMenuItem = new FBlueprintDragDropMenuItem(Context, SampleAction, MenuGrouping);

	NewMenuItem->Category = FString::Printf(TEXT("%s|%s"), *RootCategory.ToString(), *NewMenuItem->Category);
	return MakeShareable(NewMenuItem);
}

//------------------------------------------------------------------------------
TSharedPtr<FBlueprintActionMenuItem> FBlueprintActionMenuItemFactory::MakeBoundMenuItem(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo const& ActionInfo)
{
	FBlueprintActionUiSpec UiSignature = GetActionUiSignature(EditorContext, ActionInfo);

	UBlueprintNodeSpawner const* Action = ActionInfo.NodeSpawner;
	FBlueprintActionMenuItem* NewMenuItem = new FBlueprintActionMenuItem(Action, UiSignature, ActionInfo.GetBindings());

	NewMenuItem->Grouping = MenuGrouping;
	NewMenuItem->Category = FString::Printf(TEXT("%s|%s"), *RootCategory.ToString(), *NewMenuItem->Category);

	return MakeShareable(NewMenuItem);
}

//------------------------------------------------------------------------------
UBlueprint* FBlueprintActionMenuItemFactory::GetTargetBlueprint() const
{
	UBlueprint* TargetBlueprint = nullptr;
	if (Context.Blueprints.Num() > 0)
	{
		TargetBlueprint = Context.Blueprints[0];
	}
	return TargetBlueprint;
}

//------------------------------------------------------------------------------
UEdGraph* FBlueprintActionMenuItemFactory::GetTargetGraph(TWeakPtr<FBlueprintEditor> EditorContext) const
{
	UEdGraph* TargetGraph = nullptr;
	if (Context.Graphs.Num() > 0)
	{
		TargetGraph = Context.Graphs[0];
	}
	else
	{
		UBlueprint* Blueprint = GetTargetBlueprint();
		check(Blueprint != nullptr);
		
		if (Blueprint->UbergraphPages.Num() > 0)
		{
			TargetGraph = Blueprint->UbergraphPages[0];
		}
		else if (EditorContext.IsValid())
		{
			TargetGraph = EditorContext.Pin()->GetFocusedGraph();
		}
	}
	return TargetGraph;
}

//------------------------------------------------------------------------------
FBlueprintActionUiSpec FBlueprintActionMenuItemFactory::GetActionUiSignature(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo const& ActionInfo)
{
	UBlueprintNodeSpawner const* Action = ActionInfo.NodeSpawner;

	UEdGraph* TargetGraph = GetTargetGraph(EditorContext);
	Action->PrimeDefaultUiSpec(TargetGraph);

	return Action->GetUiSpec(Context, ActionInfo.GetBindings());
}


/*******************************************************************************
 * Static FBlueprintActionMenuBuilder Helpers
 ******************************************************************************/

namespace FBlueprintActionMenuBuilderImpl
{
	typedef TArray< TSharedPtr<FEdGraphSchemaAction> > MenuItemList;

	/** Defines a sub-section of the overall blueprint menu (filter, heading, etc.) */
	struct FMenuSectionDefinition
	{
	public:
		FMenuSectionDefinition(FBlueprintActionFilter const& SectionFilter, uint32 const Flags);

		/** Series of ESectionFlags, aimed at customizing how we construct this menu section */
		uint32 Flags;
		/** A filter for this section of the menu */
		FBlueprintActionFilter Filter;
		
		/** Sets the root category for menu items in this section. */
		void SetSectionHeading(FText const& RootCategory);
		/** Gets the root category for menu items in this section. */
		FText const& GetSectionHeading() const;

		/** Sets the grouping for menu items belonging to this section. */
		void SetSectionSortOrder(int32 const MenuGrouping);
		
		/**
		 * Filters the supplied action and if it passes, spawns a new 
		 * FBlueprintActionMenuItem for the specified menu (does not add the 
		 * item to the menu-builder itself).
		 *
		 * @param  EditorContext	
		 * @param  DatabaseAction	The node-spawner that the new menu item should wrap.
		 * @return An empty TSharedPtr if the action was filtered out, otherwise a newly allocated FBlueprintActionMenuItem.
		 */
		MenuItemList MakeMenuItems(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo& DatabaseAction);

		/**
		 * 
		 * 
		 * @param  EditorContext	
		 * @param  DatabaseAction	
		 * @param  Bindings	
		 * @return 
		 */
		void AddBoundMenuItems(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo& DatabaseAction, TArray<UObject*> const& Bindings, MenuItemList& MenuItemsOut);
		
		/**
		 * Clears out any consolidated properties that this may have been 
		 * tracking (so we can start a new and spawn new consolidated menu items).
		 */
		void Empty();
		
	private:
		/** In charge of spawning menu items for this section (holds category/ordering information)*/
		FBlueprintActionMenuItemFactory ItemFactory;
		/** Tracks the properties that we've already consolidated and passed (when using the ConsolidatePropertyActions flag)*/
		TMap<UProperty const*, TSharedPtr<FBlueprintDragDropMenuItem>> ConsolidatedProperties;
	};
	
	/**
	 * To offer a fallback in case this menu system is unstable on release, this
	 * method implements the old way we used collect blueprint menu actions (for
	 * both the palette and context menu).
	 *
	 * @param  MenuSection		The primary section for the FBlueprintActionMenuBuilder.
	 * @param  BlueprintEditor	
	 * @param  MenuOut  		The menu builder we want all the legacy actions appended to.
	 */
	static void AppendLegacyItems(FMenuSectionDefinition const& MenuDef, TWeakPtr<FBlueprintEditor> BlueprintEditor, FBlueprintActionMenuBuilder& MenuOut);

	/**
	 * 
	 * 
	 * @param  Context	
	 * @return 
	 */
	static TArray<UObject*> GetBindingCandidates(FBlueprintActionContext const& Context);
}

//------------------------------------------------------------------------------
FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::FMenuSectionDefinition(FBlueprintActionFilter const& SectionFilterIn, uint32 const FlagsIn)
	: Flags(FlagsIn)
	, Filter(SectionFilterIn)
	, ItemFactory(Filter.Context)
{
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::SetSectionHeading(FText const& RootCategory)
{
	ItemFactory.RootCategory = RootCategory;
}

//------------------------------------------------------------------------------
FText const& FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::GetSectionHeading() const
{
	return ItemFactory.RootCategory;
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::SetSectionSortOrder(int32 const MenuGrouping)
{
	ItemFactory.MenuGrouping = MenuGrouping;
}
// 
//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::AddBoundMenuItems(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo& DatabaseActionInfo, TArray<UObject*> const& PerspectiveBindings, MenuItemList& MenuItemsOut)
{
	UBlueprintNodeSpawner const* DatabaseAction = DatabaseActionInfo.NodeSpawner;

	TSharedPtr<FBlueprintActionMenuItem> LastMadeMenuItem;
	bool const bConsolidate = (Flags & FBlueprintActionMenuBuilder::ConsolidateBoundActions) != 0;
	
	IBlueprintNodeBinder::FBindingSet CompatibleBindings;
	// we don't want the blueprint database growing out of control with an entry 
	// for every object you could ever possibly bind to, so each 
	// UBlueprintNodeSpawner comes with an interface to test/bind through... 
	for (auto BindingIt = PerspectiveBindings.CreateConstIterator(); BindingIt;)
	{
		UObject const* BindingObj = *BindingIt;
		++BindingIt;
		bool const bIsLastBinding = !BindingIt;

		// check to see if this object can be bound to this action
		if (DatabaseAction->IsBindingCompatible(BindingObj))
		{
			// add bindings before filtering (in case tests accept/reject based off of this)
			CompatibleBindings.Add(BindingObj);
		}

		// if BoundAction is now "full" (meaning it can take any more 
		// bindings), or if this is the last binding to test...
		if ((CompatibleBindings.Num() > 0) && (!DatabaseAction->CanBindMultipleObjects() || bIsLastBinding || !bConsolidate))
		{
			// we don't want binding to mutate DatabaseActionInfo, so we clone  
			// the action info, and tack on some binding data
			FBlueprintActionInfo BoundActionInfo(DatabaseActionInfo, CompatibleBindings);

			// have to check IsFiltered() for every "fully bound" action (in
			// case there are tests that reject based off of this), we may 
			// test this multiple times per action (we have to make sure 
			// that every set of bound objects pass before folding them into
			// MenuItem)
			bool const bPassedFilter = !Filter.IsFiltered(BoundActionInfo);
			if (bPassedFilter)
			{
				if (!bConsolidate || !LastMadeMenuItem.IsValid())
				{
					LastMadeMenuItem = ItemFactory.MakeBoundMenuItem(EditorContext, BoundActionInfo);
					MenuItemsOut.Add(LastMadeMenuItem);

					if (Flags & FBlueprintActionMenuBuilder::FlattenCategoryHierarcy)
					{
						LastMadeMenuItem->Category = ItemFactory.RootCategory.ToString();
					}
				}
				else
				{
					// move these bindings over to the menu item (so we can 
					// test the next set)
					LastMadeMenuItem->AppendBindings(Filter.Context, CompatibleBindings);
				}
			}
			CompatibleBindings.Empty(); // do before we copy back over cached fields for DatabaseActionInfo

			// copy over any fields that got cached for filtering (with
			// an empty binding set)
			/*DatabaseActionInfo = FBlueprintActionInfo(BoundActionInfo, CompatibleBindings);*/
		}
	}
}

//------------------------------------------------------------------------------
FBlueprintActionMenuBuilderImpl::MenuItemList FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::MakeMenuItems(TWeakPtr<FBlueprintEditor> EditorContext, FBlueprintActionInfo& DatabaseAction)
{	
	TSharedPtr<FEdGraphSchemaAction> UnBoundMenuEntry;
	bool bPassedFilter = !Filter.IsFiltered(DatabaseAction);

	// if the caller wants to consolidate all property actions, then we have to 
	// check and see if this is one of those that needs consolidating (needs 
	// a FBlueprintDragDropMenuItem instead of a FBlueprintActionMenuItem)
	if (bPassedFilter && (Flags & FBlueprintActionMenuBuilder::ConsolidatePropertyActions))
	{
		UProperty const* ActionProperty = nullptr;
		if (UBlueprintVariableNodeSpawner const* VariableSpawner = Cast<UBlueprintVariableNodeSpawner>(DatabaseAction.NodeSpawner))
		{
			ActionProperty = VariableSpawner->GetVarProperty();
			bPassedFilter = (ActionProperty != nullptr);
		}
		else if (UBlueprintDelegateNodeSpawner const* DelegateSpawner = Cast<UBlueprintDelegateNodeSpawner>(DatabaseAction.NodeSpawner))
		{
			ActionProperty = DelegateSpawner->GetDelegateProperty();
			bPassedFilter = (ActionProperty != nullptr);
		}

		if (ActionProperty != nullptr)
		{
			if (TSharedPtr<FBlueprintDragDropMenuItem>* ConsolidatedMenuItem = ConsolidatedProperties.Find(ActionProperty))
			{
				(*ConsolidatedMenuItem)->AppendAction(DatabaseAction.NodeSpawner);
				// this menu entry has already been returned, don't need to 
				// create/insert a new one
				bPassedFilter = false;
			}
			else
			{
				TSharedPtr<FBlueprintDragDropMenuItem> NewMenuItem = ItemFactory.MakeDragDropMenuItem(DatabaseAction.NodeSpawner);
				ConsolidatedProperties.Add(ActionProperty, NewMenuItem);
				UnBoundMenuEntry = NewMenuItem;
			}
		}
	}

	if (!UnBoundMenuEntry.IsValid() && bPassedFilter)
	{
		UnBoundMenuEntry = ItemFactory.MakeActionMenuItem(EditorContext, DatabaseAction);
		if (Flags & FBlueprintActionMenuBuilder::FlattenCategoryHierarcy)
		{
			UnBoundMenuEntry->Category = ItemFactory.RootCategory.ToString();
		}
	}

	FBlueprintActionMenuBuilderImpl::MenuItemList MenuItems;
	if (UnBoundMenuEntry.IsValid())
	{
		MenuItems.Add(UnBoundMenuEntry);
	}
	AddBoundMenuItems(EditorContext, DatabaseAction, GetBindingCandidates(Filter.Context), MenuItems);

	return MenuItems;
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::FMenuSectionDefinition::Empty()
{
	ConsolidatedProperties.Empty();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilderImpl::AppendLegacyItems(FMenuSectionDefinition const& MenuDef, TWeakPtr<FBlueprintEditor> BlueprintEditor, FBlueprintActionMenuBuilder& MenuOut)
{
	FBlueprintActionFilter const&  MenuFilter  = MenuDef.Filter;
	FBlueprintActionContext const& MenuContext = MenuFilter.Context;
	
	// if this is for the context menu
	if (MenuContext.Graphs.Num() > 0)
	{
		UEdGraph* Graph = MenuContext.Graphs[0];
		UEdGraphSchema const* GraphSchema = GetDefault<UEdGraphSchema>(Graph->Schema);
		
		FBlueprintGraphActionListBuilder LegacyBuilder(Graph);
		if (MenuContext.Pins.Num() > 0)
		{
			LegacyBuilder.FromPin = MenuContext.Pins[0];
		}
		
		bool bIsContextSensitive = true;
		if (BlueprintEditor.IsValid())
		{
			bIsContextSensitive = BlueprintEditor.Pin()->GetIsContextSensitive();
			if (bIsContextSensitive)
			{
				FEdGraphSchemaAction_K2Var* SelectedVar = BlueprintEditor.Pin()->GetMyBlueprintWidget()->SelectionAsVar();
				if ((SelectedVar != nullptr) && (SelectedVar->GetProperty() != nullptr))
				{
					LegacyBuilder.SelectedObjects.Add(SelectedVar->GetProperty());
				}
			}
		}
		
		if (bIsContextSensitive)
		{
			GraphSchema->GetGraphContextActions(LegacyBuilder);
			MenuOut.Append(LegacyBuilder);
		}
		else
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);
			FBlueprintPaletteListBuilder ContextlessLegacyBuilder(Blueprint);
			UEdGraphSchema_K2::GetAllActions(ContextlessLegacyBuilder);
			MenuOut.Append(ContextlessLegacyBuilder);
		}
	}
	else if (MenuContext.Blueprints.Num() > 0)
	{
		UBlueprint* Blueprint = MenuContext.Blueprints[0];
		FBlueprintPaletteListBuilder LegacyBuilder(Blueprint, MenuDef.GetSectionHeading().ToString());
		
		UClass* ClassFilter = nullptr;
		if (MenuFilter.TargetClasses.Num() > 0)
		{
			ClassFilter = MenuFilter.TargetClasses[0];
		}
		
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FK2ActionMenuBuilder(K2Schema).GetPaletteActions(LegacyBuilder, ClassFilter);
		
		MenuOut.Append(LegacyBuilder);
	}
}

//------------------------------------------------------------------------------
static TArray<UObject*> FBlueprintActionMenuBuilderImpl::GetBindingCandidates(FBlueprintActionContext const& Context)
{
	return Context.SelectedObjects;
}

/*******************************************************************************
 * FBlueprintActionMenuBuilder
 ******************************************************************************/

//------------------------------------------------------------------------------
FBlueprintActionMenuBuilder::FBlueprintActionMenuBuilder(TWeakPtr<FBlueprintEditor> InBlueprintEditorPtr)
	: BlueprintEditorPtr(InBlueprintEditorPtr)
{
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::Empty()
{
	FGraphActionListBuilderBase::Empty();
	MenuSections.Empty();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::AddMenuSection(FBlueprintActionFilter const& Filter, FText const& Heading/* = FText::GetEmpty()*/, int32 MenuOrder/* = 0*/, uint32 const Flags/* = 0*/)
{
	using namespace FBlueprintActionMenuBuilderImpl;
	
	TSharedRef<FMenuSectionDefinition> SectionDescRef = MakeShareable(new FMenuSectionDefinition(Filter, Flags));
	SectionDescRef->SetSectionHeading(Heading);
	SectionDescRef->SetSectionSortOrder(MenuOrder);

	MenuSections.Add(SectionDescRef);
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuBuilder::RebuildActionList()
{
	using namespace FBlueprintActionMenuBuilderImpl;

	FGraphActionListBuilderBase::Empty();
	for (TSharedRef<FMenuSectionDefinition> MenuSection : MenuSections)
	{
		// clear out intermediate actions that may have been spawned (like 
		// consolidated property actions).
		MenuSection->Empty();
	}
	
	const UBlueprintEditorSettings* BlueprintSettings = GetDefault<UBlueprintEditorSettings>();
	if (!BlueprintSettings->bForceLegacyMenuingSystem)
	{
		FBlueprintActionDatabase::FActionRegistry const& ActionDatabase = FBlueprintActionDatabase::Get().GetAllActions();
		for (auto const& ActionEntry : ActionDatabase)
		{
			for (UBlueprintNodeSpawner const* NodeSpawner : ActionEntry.Value)
			{
				FBlueprintActionInfo BlueprintAction(ActionEntry.Key, NodeSpawner);

				// @TODO: could probably have a super filter that spreads across 
				//        all MenuSctions (to pair down on performance?)
				for (TSharedRef<FMenuSectionDefinition> const& MenuSection : MenuSections)
				{
					for (TSharedPtr<FEdGraphSchemaAction> MenuEntry : MenuSection->MakeMenuItems(BlueprintEditorPtr, BlueprintAction))
					{
						AddAction(MenuEntry);
					}
				}
			}
		}	
	}
	else if (MenuSections.Num() > 0)
	{
		AppendLegacyItems(*MenuSections[0], BlueprintEditorPtr, *this);
	}
}

#undef LOCTEXT_NAMESPACE
