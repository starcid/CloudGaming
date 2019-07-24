// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Slate/SlateGameResources.h"
#include "../Base/SUTPanelBase.h"
#include "../SUWindowsStyle.h"

#if !UE_SERVER

#include "SWebBrowser.h"
#include "IWebBrowserWindow.h"

class UNREALTOURNAMENT_API SUTWebBrowserPanel : public SUTPanelBase
{
	SLATE_BEGIN_ARGS(SUTWebBrowserPanel)
	: _InitialURL(TEXT("https://www.google.com"))
	, _ShowControls(true)
	, _ViewportSize(FVector2D(1920, 1080))
	, _AllowScaling(false)
	{}

	SLATE_ARGUMENT(FString, InitialURL)
	SLATE_ARGUMENT(bool, ShowControls)
	SLATE_ARGUMENT(FVector2D, ViewportSize)
	SLATE_ARGUMENT(bool, AllowScaling)
		
	SLATE_EVENT(SWebBrowser::FOnBeforeBrowse, OnBeforeBrowse)

	SLATE_EVENT(FOnBeforePopupDelegate, OnBeforePopup)
	SLATE_EVENT(FSimpleDelegate, OnLoadCompleted)
	SLATE_EVENT(FSimpleDelegate, OnLoadError)

	SLATE_END_ARGS()
	
public:
	void Construct(const FArguments& InArgs, TWeakObjectPtr<UUTLocalPlayer> InPlayerOwner);
	~SUTWebBrowserPanel();

	virtual void ConstructPanel(FVector2D ViewportSize);
	virtual void Browse(FString URL);

	virtual void OnShowPanel(TSharedPtr<SUTMenuBase> inParentWindow);
	virtual void OnHidePanel();

	virtual void ExecuteJavascript(const FString& JS) { WebBrowserPanel->ExecuteJavascript(JS); }
	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
	{
		if (WebBrowserPanel.IsValid())
		{
			WebBrowserPanel->BindUObject(Name, Object, bIsPermanent);
		}
	}

	void UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
	{
		if (WebBrowserPanel.IsValid())
		{
			WebBrowserPanel->UnbindUObject(Name, Object, bIsPermanent);
		}
	}

	void SetContent(const FString& NewContent)
	{
		if (WebBrowserPanel.IsValid())
		{
			WebBrowserPanel->LoadString(NewContent,TEXT("Test"));
		}
	}

protected:
	
	TSharedPtr<SOverlay> Overlay;

	FVector2D DesiredViewportSize;
	bool bAllowScaling;

	TSharedPtr<SVerticalBox> WebBrowserContainer;
	// The Actual Web browser panel.
	TSharedPtr<SWebBrowser> WebBrowserPanel;
	
	virtual bool BeforeBrowse(const FString& TargetURL, const FWebNavigationRequest& Request);
	virtual bool BeforePopup(FString TargetURL, FString FrameName);

	virtual void OnLoadCompleted();
	virtual void OnLoadError();

	SWebBrowser::FOnBeforeBrowse OnBeforeBrowse;

	FOnBeforePopupDelegate OnBeforePopup;

	FSimpleDelegate OnLoadCompletedDelegate;
	FSimpleDelegate OnLoadErrorDelegate;

	float GetReverseScale() const;
	bool ShowControls;

	FString InitialURL;
	FString DesiredURL;
	bool bShowWarning;

	void WarningResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	bool DisableContextMenu()
	{
		return true;
	}
};

#endif