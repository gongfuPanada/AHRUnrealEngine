// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MediaTextureFactoryNew.generated.h"


/**
 * Implements a factory for UMediaTexture objects.
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UMediaTextureFactoryNew
	: public UFactory
{
	GENERATED_UCLASS_BODY()

	/** An initial media asset to place in the newly created texture. */
	UPROPERTY()
	class UMediaAsset* InitialMediaAsset;

public:

	// UFactory Interface

	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn ) override;
	virtual bool ShouldShowInNewMenu( ) const override;
};