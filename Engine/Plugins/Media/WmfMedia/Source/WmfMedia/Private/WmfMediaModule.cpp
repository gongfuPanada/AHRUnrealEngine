// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaPrivatePCH.h"
#include "IMediaModule.h"
#include "IMediaPlayerFactory.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"

#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfplay")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "shlwapi")


DEFINE_LOG_CATEGORY(LogWmfMedia);

#define LOCTEXT_NAMESPACE "FWmfMediaModule"


/**
 * Implements the WmfMedia module.
 */
class FWmfMediaModule
	: public IModuleInterface
	, public IMediaPlayerFactory
{
public:

	/** Default constructor. */
	FWmfMediaModule()
		: Initialized(false)
	{ }

public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		// load required libraries
		IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

		if (MediaModule == nullptr)
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to load Media module"));

			return;
		}

		if (!LoadRequiredLibraries())
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to load required Windows Media Foundation libraries"));

			return;
		}

		// initialize Windows Media Foundation
		HRESULT Result = MFStartup(MF_VERSION);

		if (FAILED(Result))
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to initialize Windows Media Foundation, Error %i"), Result);

			return;
		}

		// initialize supported media formats
		SupportedFormats.Add(TEXT("3g2"), LOCTEXT("Format3g2", "3G2 Multimedia Stream"));
		SupportedFormats.Add(TEXT("3gp"), LOCTEXT("Format3gp", "3GP Video Stream"));
		SupportedFormats.Add(TEXT("3gp2"), LOCTEXT("Format3gp2", "3GPP2 Multimedia File"));
		SupportedFormats.Add(TEXT("3gpp"), LOCTEXT("Format3gpp", "3GPP Multimedia File"));
		SupportedFormats.Add(TEXT("aac"), LOCTEXT("FormatAac", "MPEG-2 Advanced Audio Coding File"));
		SupportedFormats.Add(TEXT("adts"), LOCTEXT("FormatAdts", "Audio Data Transport Stream"));
		SupportedFormats.Add(TEXT("asf"), LOCTEXT("FormatAsf", "ASF Media File"));
		SupportedFormats.Add(TEXT("avi"), LOCTEXT("FormatAvi", "Audio Video Interleave File"));
		SupportedFormats.Add(TEXT("m4v"), LOCTEXT("FormatM4v", "Apple MPEG-4 Video"));
		SupportedFormats.Add(TEXT("mov"), LOCTEXT("FormatMov", "Apple QuickTime Movie"));
		SupportedFormats.Add(TEXT("mp4"), LOCTEXT("FormatMp4", "MPEG-4 Movie"));
		SupportedFormats.Add(TEXT("sami"), LOCTEXT("FormatSami", "Synchronized Accessible Media Interchange (SAMI) File"));
		SupportedFormats.Add(TEXT("smi"), LOCTEXT("FormatSmi", "Synchronized Multimedia Integration (SMIL) File"));
		SupportedFormats.Add(TEXT("wmv"), LOCTEXT("FormatWmv", "Windows Media Video"));

		// register factory
		MediaModule->RegisterPlayerFactory(*this);

		Initialized = true;
	}

	virtual void ShutdownModule() override
	{
		if (!Initialized)
		{
			return;
		}

		Initialized = false;

		// unregister video player factory
		IMediaModule* MediaModule = FModuleManager::GetModulePtr<IMediaModule>("Media");

		if (MediaModule != nullptr)
		{
			MediaModule->UnregisterPlayerFactory(*this);
		}		

		// shutdown Windows Media Foundation
		MFShutdown();
	}

public:

	// IMediaPlayerFactory interface

	virtual TSharedPtr<IMediaPlayer> CreatePlayer() override
	{
		if (Initialized)
		{
			return MakeShareable(new FWmfMediaPlayer());
		}

		return nullptr;
	}

	virtual const FMediaFormats& GetSupportedFormats() const override
	{
		return SupportedFormats;
	}

protected:

	/**
	 * Loads all required Windows libraries.
	 *
	 * @return true on success, false otherwise.
	 */
	bool LoadRequiredLibraries()
	{
		if (LoadLibraryW(TEXT("shlwapi.dll")) == nullptr)
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to load shlwapi.dll"));

			return false;
		}

		if (LoadLibraryW(TEXT("mf.dll")) == nullptr)
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to load mf.dll"));

			return false;
		}

		if (LoadLibraryW(TEXT("mfplat.dll")) == nullptr)
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to load mfplat.dll"));

			return false;
		}

		if (LoadLibraryW(TEXT("mfplay.dll")) == nullptr)
		{
			UE_LOG(LogWmfMedia, Log, TEXT("Failed to load mfplay.dll"));

			return false;
		}

		return true;
	}

private:

	/** Whether the module has been initialized. */
	bool Initialized;

	/** The collection of supported media formats. */
	FMediaFormats SupportedFormats;
};


IMPLEMENT_MODULE(FWmfMediaModule, WmfMedia);


#undef LOCTEXT_NAMESPACE
