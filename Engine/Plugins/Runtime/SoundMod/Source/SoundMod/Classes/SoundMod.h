// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * A sound module file
 */

#include "Sound/SoundBase.h"
#include "SoundMod.generated.h"
struct FActiveSound;

UCLASS(hidecategories=Object, MinimalAPI, BlueprintType)
class USoundMod : public USoundBase
{
	GENERATED_UCLASS_BODY()

	/** If set, when played directly (not through a sound cue) the nid will be played looping. */
	UPROPERTY(EditAnywhere, Category=SoundMod)
	uint32 bLooping:1;

	/** The mod file data */
	FByteBulkData				RawData;

private:

	/** Memory containing the data copied from the compressed bulk data */
	uint8*	ResourceData;

public:	
	// Begin UObject interface. 
	virtual void Serialize(FArchive& Ar) override;
	// End UObject interface. 

	// Begin USoundBase interface.
	virtual bool IsPlayable() const override;
	virtual void Parse(class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances) override;
	virtual float GetMaxAudibleDistance() override;
	// End USoundBase interface.
};

