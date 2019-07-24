// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "EngineFontServices.h"

#include "BatchedElements.h"
#include "RenderResource.h"

namespace ERenderObjectType
{
	const FName TextureObject = FName(TEXT("HUDRenderObject_Texture"));
	const FName TextObject = FName(TEXT("HUDRenderObject_Text"));
}

UUTHUDWidget::UUTHUDWidget(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIgnoreHUDBaseColor = false;
	Opacity = 1.0f;
	Origin = FVector2D(0.0f, 0.0f);
	ScreenPosition = FVector2D(0.0f, 0.0f);
	LineupPositionOffset = FVector2D(0.0f, 0.0f);
	bScaleByDesignedResolution = true;
	bMaintainAspectRatio = true;
	DesignedResolution = 720;
	UTHUDOwner = nullptr;
	bIgnoreHUDOpacity = false;
	bShouldKickBack = true;
}

DECLARE_CYCLE_STAT(TEXT("CanvasTextItem Time"), STAT_Canvas_TextItemTime, STATGROUP_Canvas);
void FUTCanvasTextItem::Draw(FCanvas* InCanvas)
{
	SCOPE_CYCLE_COUNTER(STAT_Canvas_TextItemTime);

	if (InCanvas == NULL || Font == NULL || Text.IsEmpty())
	{
		return;
	}

	if (!FontRenderInfo.bClipText && WordWrapper.IsValid())
	{
		// word wrap
		FTextSizingParameters Parms(Font, Scale.X, Scale.Y);
		Parms.DrawXL = (WrapXL > 0.0f) ? WrapXL : (InCanvas->GetRenderTarget()->GetSizeXY().X - Position.X);
		Parms.DrawYL = InCanvas->GetRenderTarget()->GetSizeXY().Y;
		TArray<FWrappedStringElement> Lines;

		UCanvas::WrapString(*WordWrapper, Parms, 0, *Text.ToString(), Lines);

		FString NewText;
		for (int32 i = 0; i < Lines.Num(); i++)
		{
			if (i > 0)
			{
				NewText += TEXT("\n");
			}
			NewText += Lines[i].Value;
		}
		Text = FText::FromString(NewText);
	}
	
	bool bHasShadow = ShadowOffset.Size() != 0.0f;
	if (FontRenderInfo.bEnableShadow && !bHasShadow)
	{
		EnableShadow(FLinearColor::Black);
		bHasShadow = true;
	}
	if (Font->ImportOptions.bUseDistanceFieldAlpha)
	{
		// convert blend mode to distance field type
		switch (BlendMode)
		{
		case SE_BLEND_Translucent:
			BlendMode = (bHasShadow || FontRenderInfo.bEnableShadow) ? SE_BLEND_TranslucentDistanceFieldShadowed : SE_BLEND_TranslucentDistanceField;
			break;
		case SE_BLEND_Masked:
			BlendMode = (bHasShadow || FontRenderInfo.bEnableShadow) ? SE_BLEND_MaskedDistanceFieldShadowed : SE_BLEND_MaskedDistanceField;
			break;
		}

		// we don't need to use the double draw method because the shader will do it better
		FontRenderInfo.bEnableShadow = true;
		bHasShadow = false;
	}

	CharIncrement = ((float)Font->Kerning + HorizSpacingAdjust) * Scale.X;
	DrawnSize.Y = Font->GetMaxCharHeight() * Scale.Y;

	FVector2D DrawPos(Position.X, Position.Y);

	// If we are centering the string or we want to fix stereoscopic rending issues we need to measure the string
	if ((bCentreX || bCentreY) || (!bDontCorrectStereoscopic))
	{
		FTextSizingParameters Parameters(Font, Scale.X, Scale.Y);
		UCanvas::CanvasStringSize(Parameters, *Text.ToString());

		// Calculate the offset if we are centering
		if (bCentreX || bCentreY)
		{
			// Note we drop the fraction after the length divide or we can end up with coords on 1/2 pixel boundaries
			if (bCentreX)
			{
				DrawPos.X = DrawPos.X - (int32)(Parameters.DrawXL / 2);
			}
			if (bCentreY)
			{
				DrawPos.Y = DrawPos.Y - (int32)(Parameters.DrawYL / 2);
			}
		}

		// Check if we want to correct the stereo3d issues - if we do, render the correction now
		bool CorrectStereo = !bDontCorrectStereoscopic  && GEngine->IsStereoscopic3D();
		if (CorrectStereo)
		{
			FVector2D StereoOutlineBoxSize(2.0f, 2.0f);
			TileItem.MaterialRenderProxy = GEngine->RemoveSurfaceMaterial->GetRenderProxy(false);
			TileItem.Position = DrawPos - StereoOutlineBoxSize;
			FVector2D CorrectionSize = FVector2D(Parameters.DrawXL, Parameters.DrawYL) + StereoOutlineBoxSize + StereoOutlineBoxSize;
			TileItem.Size = CorrectionSize;
			TileItem.bFreezeTime = true;
			TileItem.Draw(InCanvas);
		}
	}

	FLinearColor DrawColor;
	BatchedElements = NULL;
	if (bOutlined && !FontRenderInfo.GlowInfo.bEnableGlow) // efficient distance field glow takes priority
	{
		DrawColor = OutlineColor;
		DrawColor.A *= InCanvas->AlphaModulate;
		UTDrawStringInternal(InCanvas, DrawPos + FVector2D(-1.0f, -1.0f), DrawColor);
		UTDrawStringInternal(InCanvas, DrawPos + FVector2D(-1.0f, 1.0f), DrawColor);
		UTDrawStringInternal(InCanvas, DrawPos + FVector2D(1.0f, 1.0f), DrawColor);
		UTDrawStringInternal(InCanvas, DrawPos + FVector2D(1.0f, -1.0f), DrawColor);
	}
	// If we have a shadow - draw it now
	if (bHasShadow)
	{
		DrawColor = ShadowColor;
		// Copy the Alpha from the shadow otherwise if we fade the text the shadow wont fade - which is almost certainly not what we will want.
		DrawColor.A = Color.A;
		DrawColor.A *= InCanvas->AlphaModulate;
		UTDrawStringInternal(InCanvas, DrawPos + ShadowOffset, DrawColor);
	}
	DrawColor = Color;
	DrawColor.A *= InCanvas->AlphaModulate;
	// TODO: we need to pass the shadow color and direction in the distance field case... (currently engine support is missing)
	UTDrawStringInternal(InCanvas, DrawPos, DrawColor);
}

void FUTCanvasTextItem::UTDrawStringInternal(class FCanvas* InCanvas, const FVector2D& DrawPos, const FLinearColor& InColour)
{
	// TODO: we may have to implement this if we want to use composite fonts with proper glow, distance field shadow, et al
	// for now, just redirect to the flawed engine implementation
	if (GetFontCacheType() == EFontCacheType::Runtime)
	{
		BlendMode = SE_BLEND_TranslucentAlphaOnly;
		DrawStringInternal_RuntimeCache(InCanvas, DrawPos, InColour);
		return;
	}

	DrawnSize = FVector2D(EForceInit::ForceInitToZero);
	FVector2D CurrentPos = FVector2D(EForceInit::ForceInitToZero);
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();
	FTexture* LastTexture = NULL;
	UTexture2D* Tex = NULL;
	FVector2D InvTextureSize(1.0f, 1.0f);
	const TArray< TCHAR >& Chars = Text.ToString().GetCharArray();

	// allocate extra (overlapping) space to draw distance field effects, if enabled
	float ExtraXSpace = 0.0f;
	float ExtraYSpace = 0.0f;
	if (Font->ImportOptions.bUseDistanceFieldAlpha)
	{
		if (FontRenderInfo.bEnableShadow || BlendMode == SE_BLEND_MaskedDistanceFieldShadowed || BlendMode == SE_BLEND_TranslucentDistanceFieldShadowed)
		{
			// TODO: should be based on shadow settings, but those are currently hardcoded in FBatchedElements
			ExtraXSpace = 2.0f;
			ExtraYSpace = 2.0f;
		}
		if (FontRenderInfo.GlowInfo.bEnableGlow)
		{
			// we can calculate the maximum distance a glowing pixel will be by multiplying
			// the distance field scan radius (font height / 2) by the extremes of the glow radii settings
			// this lets us get away with less padding in the texture when the glow being applied
			// won't extend to the maximum values

			float GlowRadius = Font->ImportOptions.Height * 0.5;
			// < 0.5 is outside the glyph so that's all we care about
			float EdgeRadius = FMath::Min<float>( FMath::Min<float>(FontRenderInfo.GlowInfo.GlowInnerRadius.X, FontRenderInfo.GlowInfo.GlowInnerRadius.Y),
												FMath::Min<float>(FontRenderInfo.GlowInfo.GlowOuterRadius.X, FontRenderInfo.GlowInfo.GlowOuterRadius.Y) );
			if (EdgeRadius < 0.5f)
			{
				GlowRadius = FMath::CeilToFloat(GlowRadius * (0.5f - EdgeRadius) * 2.0f);
				ExtraXSpace = FMath::Max<float>(ExtraXSpace, GlowRadius);
				ExtraYSpace = FMath::Max<float>(ExtraYSpace, GlowRadius);
			}
		}
		// clamp by the padding in the texture (prefer box-y look over bleeding to adjacent glyphs)
		ExtraXSpace = FMath::Min<float>(ExtraXSpace, FMath::TruncToFloat((Font->ImportOptions.XPadding - 1.0f) * 0.5f));
		ExtraYSpace = FMath::Min<float>(ExtraYSpace, FMath::TruncToFloat((Font->ImportOptions.YPadding - 1.0f) * 0.5f));
	}

	// Draw all characters in string.
	for (int32 i = 0; i < Chars.Num() - 1; i++)
	{
		int32 Ch = (int32)Font->RemapChar(Chars[i]);

		// Skip invalid characters.
		if (!Font->Characters.IsValidIndex(Ch))
		{
			continue;
		}

		const FFontCharacter& Char = Font->Characters[Ch];

		if (DrawnSize.Y == 0)
		{
			// We have a valid character so initialize vertical DrawnSize
			DrawnSize.Y = Font->GetMaxCharHeight() * Scale.Y;
		}

		if (FChar::IsLinebreak(Chars[i]))
		{
			// Set current character offset to the beginning of next line.
			CurrentPos.X = 0.0f;
			CurrentPos.Y += Font->GetMaxCharHeight() * Scale.Y;

			// Increase the vertical DrawnSize
			DrawnSize.Y += Font->GetMaxCharHeight() * Scale.Y;

			// Don't draw newline character
			continue;
		}

		if (Font->Textures.IsValidIndex(Char.TextureIndex) &&
			(Tex = Font->Textures[Char.TextureIndex]) != NULL &&
			Tex->Resource != NULL)
		{
			if (LastTexture != Tex->Resource || BatchedElements == NULL)
			{
				FBatchedElementParameters* TempBatchedElementParameters = NULL;
				BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Triangle, TempBatchedElementParameters, Tex->Resource, BlendMode, FontRenderInfo.GlowInfo);
				check(BatchedElements != NULL);
				// trade-off between memory and performance by pre-allocating more reserved space 
				// for the triangles/vertices of the batched elements used to render the text tiles
				//BatchedElements->AddReserveTriangles(TextLen*2,Tex->Resource,BlendMode);
				//BatchedElements->AddReserveVertices(TextLen*4);

				FIntPoint ImportedTextureSize = Tex->GetImportedSize();
				InvTextureSize.X = 1.0f / (float)ImportedTextureSize.X;
				InvTextureSize.Y = 1.0f / (float)ImportedTextureSize.Y;
			}
			LastTexture = Tex->Resource;

			const float X = CurrentPos.X + DrawPos.X - ExtraXSpace * Scale.X;
			const float Y = CurrentPos.Y + DrawPos.Y + (Char.VerticalOffset - ExtraYSpace) * Scale.Y;
			float SizeX = (Char.USize + ExtraXSpace * 2.0f) * Scale.X;
			const float SizeY = (Char.VSize + ExtraYSpace * 2.0f) * Scale.Y;
			const float U = (Char.StartU - ExtraXSpace) * InvTextureSize.X;
			const float V = (Char.StartV - ExtraYSpace) * InvTextureSize.Y;
			const float SizeU = (Char.USize + ExtraXSpace * 2.0f) * InvTextureSize.X;
			const float SizeV = (Char.VSize + ExtraYSpace * 2.0f) * InvTextureSize.Y;

			float Left, Top, Right, Bottom;
			Left = X * Depth;
			Top = Y * Depth;
			Right = (X + SizeX) * Depth;
			Bottom = (Y + SizeY) * Depth;

			int32 V00 = BatchedElements->AddVertex(
				FVector4(Left, Top, 0.f, Depth),
				FVector2D(U, V),
				InColour,
				HitProxyId);
			int32 V10 = BatchedElements->AddVertex(
				FVector4(Right, Top, 0.0f, Depth),
				FVector2D(U + SizeU, V),
				InColour,
				HitProxyId);
			int32 V01 = BatchedElements->AddVertex(
				FVector4(Left, Bottom, 0.0f, Depth),
				FVector2D(U, V + SizeV),
				InColour,
				HitProxyId);
			int32 V11 = BatchedElements->AddVertex(
				FVector4(Right, Bottom, 0.0f, Depth),
				FVector2D(U + SizeU, V + SizeV),
				InColour,
				HitProxyId);

			BatchedElements->AddTriangle(V00, V10, V11, Tex->Resource, BlendMode, FontRenderInfo.GlowInfo);
			BatchedElements->AddTriangle(V00, V11, V01, Tex->Resource, BlendMode, FontRenderInfo.GlowInfo);

			// if we have another non-whitespace character to render, add the font's kerning.
			if (Chars[i + 1] && !FChar::IsWhitespace(Chars[i + 1]))
			{
				SizeX += CharIncrement;
			}

			// Update the current rendering position
			CurrentPos.X += SizeX - (ExtraXSpace * 2.0f) * Scale.X;

			// Increase the Horizontal DrawnSize
			if (CurrentPos.X > DrawnSize.X)
			{
				DrawnSize.X = CurrentPos.X;
			}
		}
	}
}

void UUTHUDWidget::InitializeWidget(AUTHUD* InHUD)
{
	UTHUDOwner = InHUD;
	RealOrigin = Origin;
	for (TFieldIterator<UStructProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		UStructProperty* Prop = NULL;
		float PropRenderPriority = 0.0;
		
		const FName CPPName = PropIt->Struct->GetFName();
		if ( CPPName == ERenderObjectType::TextureObject) 
		{
			// Get the object.
			Prop = *PropIt;
			FHUDRenderObject_Texture* PtrToProperty = Prop->ContainerPtrToValuePtr<FHUDRenderObject_Texture>(this,0);
			if (PtrToProperty)
			{
				PropRenderPriority = PtrToProperty->RenderPriority;
			}
		}
		if ( CPPName == ERenderObjectType::TextObject) 
		{
			// Get the object.
			Prop = *PropIt;
			FHUDRenderObject_Text* PtrToProperty = Prop->ContainerPtrToValuePtr<FHUDRenderObject_Text>(this, 0);
			if (PtrToProperty)
			{
				PropRenderPriority = PtrToProperty->RenderPriority;
			}
	
		}

		if (Prop)
		{
			int32 InsertPosition = -1;
			for (int32 i=0; i < RenderObjectList.Num(); i++)
			{
				FHUDRenderObject* ObjPtr = RenderObjectList[i]->ContainerPtrToValuePtr<FHUDRenderObject>(this, 0);
				if (ObjPtr)
				{
					if (PropRenderPriority < ObjPtr->RenderPriority)
					{
						InsertPosition = i;
						break;
					}
				}
			}

			if (InsertPosition >= 0)
			{
				RenderObjectList.Insert(Prop, InsertPosition);
			}
			else
			{
				RenderObjectList.Add(Prop);
			}
		}
	}
}

UCanvas* UUTHUDWidget::GetCanvas()
{
	return Canvas;
}

FVector2D UUTHUDWidget::GetRenderPosition()
{
	return RenderPosition;
}

FVector2D UUTHUDWidget::GetRenderSize()
{
	return RenderSize;
}

float UUTHUDWidget::GetRenderScale()
{
	return RenderScale;
}

bool UUTHUDWidget::IsHidden()
{
	return bHidden;
}

void UUTHUDWidget::SetHidden(bool bIsHidden)
{
	bHidden = bIsHidden;
}

bool UUTHUDWidget::ShouldDraw_Implementation(bool bShowScores)
{
	return UTGameState && (UTGameState->HasMatchStarted() || (UTHUDOwner && UTHUDOwner->UTPlayerOwner && UTHUDOwner->UTPlayerOwner->UTPlayerState && UTHUDOwner->UTPlayerOwner->UTPlayerState->bIsWarmingUp)) && UTCharacterOwner && !UTCharacterOwner->IsDead() && !bShowScores && (UTGameState->GetMatchState() != MatchState::MatchIntermission) && !UTGameState->HasMatchEnded();
}

void UUTHUDWidget::PreDraw(float DeltaTime, AUTHUD* InUTHUDOwner, UCanvas* InCanvas, FVector2D InCanvasCenter)
{
	UTHUDOwner = InUTHUDOwner;
	if (UTHUDOwner != NULL)
	{
		Origin = bShouldKickBack ? RealOrigin + UTHUDOwner->CurrentHUDImpulse : RealOrigin;
		UTPlayerOwner = UTHUDOwner->UTPlayerOwner;
		if (UTPlayerOwner != NULL)
		{
			AUTPlayerState* PS = UTPlayerOwner->UTPlayerState;
			static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
			UTCharacterOwner = (PS && PS->bOnlySpectator && UTPlayerOwner->PlayerCameraManager && (UTPlayerOwner->PlayerCameraManager->CameraStyle == NAME_FreeCam))
									? NULL
									: Cast<AUTCharacter>(UTPlayerOwner->GetViewTarget());
		}

		UTGameState = UTHUDOwner->GetWorld()->GetGameState<AUTGameState>();
	}

	Canvas = InCanvas;
	CanvasCenter = InCanvasCenter;
	AspectScale = Size.Y > 0 ? Size.X / Size.Y : 1.0;

	FVector2D CurrentScreenPosition = ScreenPosition;
	if (UTGameState && UTGameState->IsLineUpActive())
	{
		CurrentScreenPosition += LineupPositionOffset;
	}
	// Figure out the initial position.
	RenderPosition.X = Canvas->ClipX * CurrentScreenPosition.X;
	RenderPosition.Y = Canvas->ClipY * CurrentScreenPosition.Y;
	RenderScale = (bScaleByDesignedResolution) ? Canvas->ClipY / DesignedResolution : 1.0f;
	RenderScale *= GetDrawScaleOverride();

	// Apply any scaling
	RenderSize.Y = Size.Y * RenderScale;
	if (Size.X > 0)
	{
		RenderSize.X = (bMaintainAspectRatio ?  RenderSize.Y * AspectScale : Size.X * RenderScale);
	}
	RenderPosition.X += (Position.X * RenderScale) - (RenderSize.X * Origin.X);
	RenderPosition.Y += (Position.Y * RenderScale) - (RenderSize.Y * Origin.Y);
}

void UUTHUDWidget::Draw_Implementation(float DeltaTime)
{
	DrawAllRenderObjects(DeltaTime, FVector2D(0,0));
}

void UUTHUDWidget::PostDraw(float RenderedTime)
{
	 if (bShowBounds)
	 {
		Canvas->SetDrawColor(255,255,0,255);
		Canvas->K2_DrawBox(RenderPosition, RenderSize, 1.0);
	 }

	LastRenderTime = RenderedTime;
	Canvas = NULL;
	UTCharacterOwner = NULL;
}

void UUTHUDWidget::DrawAllRenderObjects(float RenderedTime, FVector2D DrawOffset)
{
	for (int32 i=0; i < RenderObjectList.Num(); i++)
	{
		UStructProperty* Prop = RenderObjectList[i]; 
		const FName CPPName = Prop->Struct->GetFName();
		if ( CPPName == ERenderObjectType::TextureObject) 
		{
			// Get the object.
			FHUDRenderObject_Texture* PtrToTexture = RenderObjectList[i]->ContainerPtrToValuePtr<FHUDRenderObject_Texture>(this,0);
			if (PtrToTexture)
			{
				RenderObj_Texture(*PtrToTexture, DrawOffset);
			}
		}
		else if ( CPPName == ERenderObjectType::TextObject) 
		{
			// Get the object.
			FHUDRenderObject_Text* PtrToText = RenderObjectList[i]->ContainerPtrToValuePtr<FHUDRenderObject_Text>(this, 0);
			if (PtrToText)
			{
				RenderObj_Text(*PtrToText, DrawOffset);
			}
		}
	}
}

FVector2D UUTHUDWidget::DrawText(FText Text, float X, float Y, UFont* Font, float TextScale, float DrawOpacity, FLinearColor DrawColor, ETextHorzPos::Type TextHorzAlignment, ETextVertPos::Type TextVertAlignment)
{
	return DrawText(Text, X, Y, Font, false, FVector2D::ZeroVector, FLinearColor::Black, false, FLinearColor::Black, TextScale, DrawOpacity, DrawColor, FLinearColor(0.0f,0.0f,0.0f,0.0f), TextHorzAlignment, TextVertAlignment);
}

FVector2D UUTHUDWidget::DrawText(FText Text, float X, float Y, UFont* Font, FLinearColor OutlineColor, float TextScale, float DrawOpacity, FLinearColor DrawColor, ETextHorzPos::Type TextHorzAlignment, ETextVertPos::Type TextVertAlignment)
{
	return DrawText(Text, X, Y, Font, false, FVector2D::ZeroVector, FLinearColor::Black, true, OutlineColor, TextScale, DrawOpacity, DrawColor, FLinearColor(0.0f,0.0f,0.0f,0.0f), TextHorzAlignment, TextVertAlignment);
}

FVector2D UUTHUDWidget::DrawText(FText Text, float X, float Y, UFont* Font, FVector2D ShadowDirection, FLinearColor ShadowColor, float TextScale, float DrawOpacity, FLinearColor DrawColor, ETextHorzPos::Type TextHorzAlignment, ETextVertPos::Type TextVertAlignment)
{
	return DrawText(Text, X, Y, Font, true, ShadowDirection, ShadowColor, false, FLinearColor::Black, TextScale, DrawOpacity, DrawColor, FLinearColor(0.0f,0.0f,0.0f,0.0f), TextHorzAlignment, TextVertAlignment);
}
FVector2D UUTHUDWidget::DrawText(FText Text, float X, float Y, UFont* Font, FVector2D ShadowDirection, FLinearColor ShadowColor, FLinearColor OutlineColor, float TextScale, float DrawOpacity, FLinearColor DrawColor, ETextHorzPos::Type TextHorzAlignment, ETextVertPos::Type TextVertAlignment)
{
	return DrawText(Text, X, Y, Font, true, ShadowDirection, ShadowColor, true, OutlineColor, TextScale, DrawOpacity, DrawColor, FLinearColor(0.0f,0.0f,0.0f,0.0f), TextHorzAlignment, TextVertAlignment);
}
FVector2D UUTHUDWidget::DrawText(FText Text, float X, float Y, UFont* Font, const FFontRenderInfo& RenderInfo, float TextScale, float DrawOpacity, FLinearColor DrawColor, ETextHorzPos::Type TextHorzAlignment, ETextVertPos::Type TextVertAlignment)
{
	return DrawText(Text, X, Y, Font, false, FVector2D::ZeroVector, FLinearColor::Black, false, FLinearColor::Black, TextScale, DrawOpacity, DrawColor, FLinearColor(0.0f,0.0f,0.0f,0.0f), TextHorzAlignment, TextVertAlignment, RenderInfo);
}

FVector2D UUTHUDWidget::DrawText(FText Text, float X, float Y, UFont* Font, bool bDrawShadow, FVector2D ShadowDirection, FLinearColor ShadowColor, bool bDrawOutline, FLinearColor OutlineColor, float TextScale, float DrawOpacity, FLinearColor DrawColor, FLinearColor BackColor, ETextHorzPos::Type TextHorzAlignment, ETextVertPos::Type TextVertAlignment, const FFontRenderInfo& RenderInfo)
{
	float XL = 0.0f, YL = 0.0f;
	FVector2D TextSize;
	if (Font && !Text.IsEmpty())
	{
	/*	if ((Font != UTHUDOwner->TinyFont)
			&& (Font != UTHUDOwner->SmallFont)
			&& (Font != UTHUDOwner->MediumFont)
			&& (Font != UTHUDOwner->LargeFont)
			&& (Font != UTHUDOwner->HugeFont)
			&& (Font != UTHUDOwner->NumberFont))
		{
			UE_LOG(UT, Warning, TEXT("%s draw text with font %s"), *GetName(), *Font->GetName());
		}*/
		
		const bool bHasLineBreak = Text.ToString().Contains(TEXT("\n"));
		// canvas code is broken for strings with line breaks, split them manually
		TArray<FString> Pieces;
		if (bHasLineBreak)
		{
			Text.ToString().ParseIntoArrayLines(Pieces);
			for (const FString& Next : Pieces)
			{
				float TestXL, TestYL;
				Canvas->StrLen(Font, Next, TestXL, TestYL);
				XL = FMath::Max<float>(TestXL, XL);
				YL += TestYL;
			}
		}
		else
		{
			Canvas->StrLen(Font, Text.ToString(), XL, YL);
		}
		TextSize = FVector2D(XL,YL); // Save for Later

		if (bScaleByDesignedResolution)
		{
			X *= RenderScale;
			Y *= RenderScale;
		}
		const float TextScaling = bScaleByDesignedResolution ? RenderScale*TextScale : TextScale;

		if (TextHorzAlignment == ETextHorzPos::Center && bHasLineBreak)
		{
			TGuardValue<bool> ScaleGuard(bScaleByDesignedResolution, false);
			for (const FString& Next : Pieces)
			{
				FVector2D DrawnSize = DrawText(FText::FromString(Next), X, Y, Font, bDrawShadow, ShadowDirection, ShadowColor, bDrawOutline, OutlineColor, TextScaling, DrawOpacity, DrawColor, BackColor, TextHorzAlignment, TextVertAlignment, RenderInfo);
				Y += DrawnSize.Y * TextScaling * ((TextVertAlignment == ETextVertPos::Center) ? 0.5f : 1.0f);
			}
			return TextSize;
		}

		FVector2D RenderPos = FVector2D(RenderPosition.X + X,RenderPosition.Y + Y);
		// Handle Justification
		if (TextHorzAlignment != ETextHorzPos::Left || TextVertAlignment != ETextVertPos::Top )
		{
			XL *= TextScaling;
			YL *= TextScaling;

			if (TextHorzAlignment != ETextHorzPos::Left)
			{
				RenderPos.X -= TextHorzAlignment == ETextHorzPos::Right ? XL : XL * 0.5f;
			}
			if (TextVertAlignment != ETextVertPos::Top)
			{
				RenderPos.Y -= TextVertAlignment == ETextVertPos::Bottom ? YL : YL * 0.5f;
			}
		}

		if (BackColor.A > 0.0f)	// Don't even bother if the alpha is 0
		{
			Canvas->DrawColor = BackColor.ToFColor(true);
			Canvas->DrawColor.A = uint8(255.0f * BackColor.A);
			Canvas->DrawTile(Canvas->DefaultTexture, RenderPos.X, RenderPos.Y, TextSize.X * TextScale * 1.025, TextSize.Y * TextScale, 0,0,1,1,EBlendMode::BLEND_Translucent);
		}

		DrawColor.A = Opacity * DrawOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->WidgetOpacity);
		Canvas->DrawColor = DrawColor.ToFColor(false);

		if (!WordWrapper.IsValid())
		{
			WordWrapper = MakeShareable(new FCanvasWordWrapper());
		}
		FUTCanvasTextItem TextItem(RenderPos, Text, Font, DrawColor, WordWrapper);
		TextItem.FontRenderInfo = RenderInfo;

		if (bDrawOutline)
		{
			TextItem.bOutlined = true;
			TextItem.OutlineColor = OutlineColor;
			TextItem.OutlineColor.A *= DrawOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->WidgetOpacity);
		}

		if (bDrawShadow)
		{
			ShadowColor.A *= DrawOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->WidgetOpacity);
			TextItem.EnableShadow(ShadowColor, ShadowDirection);
		}

		TextItem.Scale = FVector2D(TextScaling, TextScaling);
		Canvas->DrawItem(TextItem);
	}

	return TextSize;
}

void UUTHUDWidget::DrawTexture(UTexture* Texture, float X, float Y, float Width, float Height, float U, float V, float UL, float VL, float DrawOpacity, FLinearColor DrawColor, FVector2D RenderOffset, float Rotation, FVector2D RotPivot)
{
	float ImageAspectScale = Height > 0 ? Width / Height : 0.0f;

	if (Texture && Texture->Resource )
	{
		if (bScaleByDesignedResolution)
		{
			X *= RenderScale;
			Y *= RenderScale;
			
			Height = Height * RenderScale;
			Width = (bMaintainAspectRatio) ? (Height * ImageAspectScale) : Width * RenderScale;
		}

		FVector2D RenderPos = FVector2D(RenderPosition.X + X - (Width * RenderOffset.X), RenderPosition.Y + Y - (Height * RenderOffset.Y));

		U = U / Texture->Resource->GetSizeX();
		V = V / Texture->Resource->GetSizeY();
		UL = U + (UL / Texture->Resource->GetSizeX());
		VL = V + (VL / Texture->Resource->GetSizeY());

		DrawColor.A = Opacity * DrawOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->WidgetOpacity);

		FCanvasTileItem ImageItem(RenderPos, Texture->Resource, FVector2D(Width, Height), FVector2D(U,V), FVector2D(UL,VL), DrawColor);
		ImageItem.Rotation = FRotator(0,Rotation,0);
		ImageItem.PivotPoint = RotPivot;
		ImageItem.BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
		Canvas->DrawItem( ImageItem );
	}
}

void UUTHUDWidget::DrawMaterial( UMaterialInterface* Material, float X, float Y, float Width, float Height, float MaterialU, float MaterialV, float MaterialUWidth, float MaterialVHeight, float DrawOpacity, FLinearColor DrawColor, FVector2D RenderOffset, float Rotation, FVector2D RotPivot)
{
	if (Material)
	{
		if (bScaleByDesignedResolution)
		{
			X *= RenderScale;
			Y *= RenderScale;
			
			Width = (bMaintainAspectRatio) ? ((Height * RenderScale) * AspectScale) : Width * RenderScale;
			Height = Height * RenderScale;
		}

		FVector2D RenderPos = FVector2D(RenderPosition.X + X - (Width * RenderOffset.X), RenderPosition.Y + Y - (Height * RenderOffset.Y));
		FCanvasTileItem MaterialItem( RenderPos, Material->GetRenderProxy(0), FVector2D( Width, Height) , FVector2D( MaterialU, MaterialV ), FVector2D( MaterialU+MaterialUWidth, MaterialV +MaterialVHeight));

		DrawColor.A = Opacity * DrawOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->WidgetOpacity);
		MaterialItem.SetColor(DrawColor);
		MaterialItem.Rotation = FRotator(0, Rotation, 0);
		MaterialItem.PivotPoint = RotPivot;
		Canvas->DrawColor.A *= DrawOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->WidgetOpacity);
		Canvas->DrawItem( MaterialItem );
	}
}

FLinearColor UUTHUDWidget::ApplyHUDColor(FLinearColor DrawColor)
{
	if (!bIgnoreHUDBaseColor)
	{
		DrawColor *= UTHUDOwner->GetBaseHUDColor();
	}
	return DrawColor;
}

void UUTHUDWidget::RenderObj_Texture(FHUDRenderObject_Texture& TextureObject, FVector2D DrawOffset)
{
	FVector2D NewRenderSize = FVector2D(TextureObject.GetWidth(), TextureObject.GetHeight());
	RenderObj_TextureAt(TextureObject, (TextureObject.Position.X + DrawOffset.X), (TextureObject.Position.Y + DrawOffset.Y), NewRenderSize.X, NewRenderSize.Y);
}

void UUTHUDWidget::RenderObj_TextureAtWithRotation(FHUDRenderObject_Texture& TextureObject, FVector2D Pos, float Rotation)
{
	float OldRotation = TextureObject.Rotation;
	TextureObject.Rotation += Rotation;
	RenderObj_Texture(TextureObject, Pos);
	TextureObject.Rotation  = OldRotation;
}


void UUTHUDWidget::RenderObj_TextureAt(FHUDRenderObject_Texture& TextureObject, float X, float Y, float Width, float Height)
{
	if (TextureObject.bHidden || TextureObject.Atlas == NULL || Width <= 0.0 || Height <= 0.0) return;

	FLinearColor RenderColor = TextureObject.RenderColor;
	if (TextureObject.bUseTeamColors && UTHUDOwner && UTHUDOwner->UTPlayerOwner)
	{
		if (UTGameState && UTGameState->bTeamGame)
		{
			uint8 TeamIdx = UTHUDOwner->UTPlayerOwner->GetTeamNum();

			//If spectating, get team of the viewed player
			if (TeamIdx == 255)
			{
				AUTCharacter* ViewedUTC = Cast<AUTCharacter>(UTHUDOwner->UTPlayerOwner->GetViewTarget());
				if (ViewedUTC != nullptr)
				{
					TeamIdx = ViewedUTC->GetTeamNum();
				}
			}

			if (TeamIdx < TextureObject.TeamColorOverrides.Num())
			{
				RenderColor = TextureObject.TeamColorOverrides[TeamIdx];
			}
			else
			{
				RenderColor = UTHUDOwner->GetWidgetTeamColor();
			}
		}
	}

	float NewOpacity = bIgnoreHUDOpacity ? 1.0f : (UTHUDOwner->GetHUDWidgetOpacity() * (TextureObject.bIsBorderElement ? UTHUDOwner->GetHUDWidgetBorderOpacity() : 1.0f) * (TextureObject.bIsSlateElement ? UTHUDOwner->GetHUDWidgetSlateOpacity() : 1.0f)); 

	DrawTexture(TextureObject.Atlas, 
						X, 
						Y, 
						Width, 
						Height,
						TextureObject.UVs.U, TextureObject.UVs.V, TextureObject.UVs.UL, TextureObject.UVs.VL,
						TextureObject.RenderOpacity * NewOpacity,
						RenderColor, 
						TextureObject.RenderOffset, 
						TextureObject.Rotation, 
						TextureObject.RotPivot);
}

FVector2D UUTHUDWidget::RenderObj_Text(FHUDRenderObject_Text& TextObject, FVector2D DrawOffset)
{
	return RenderObj_TextAt(TextObject, TextObject.Position.X + DrawOffset.X, TextObject.Position.Y + DrawOffset.Y);
}
FVector2D UUTHUDWidget::RenderObj_TextAt(FHUDRenderObject_Text& TextObject, float X, float Y)
{
	FText TextToRender = TextObject.Text;
	if (TextObject.GetTextDelegate.IsBound())
	{
		TextToRender = TextObject.GetTextDelegate.Execute();
	}

	if (TextObject.bHidden || TextToRender.IsEmpty() || TextObject.Font == NULL) return FVector2D(0,0);
	
	return DrawText(TextToRender, 
				X, 
				Y, 
				TextObject.Font, 
				TextObject.bDrawShadow, 
				TextObject.ShadowDirection, 
				TextObject.ShadowColor,
				TextObject.bDrawOutline, 
				TextObject.OutlineColor, 
				TextObject.TextScale, 
				TextObject.RenderOpacity * (bIgnoreHUDOpacity ? 1.0f : UTHUDOwner->GetHUDWidgetOpacity()), 
				TextObject.RenderColor, 
				TextObject.BackgroundColor,
				TextObject.HorzPosition, 
				TextObject.VertPosition);
}

float UUTHUDWidget::GetDrawScaleOverride()
{
	return (UTHUDOwner) ? UTHUDOwner->GetHUDWidgetScaleOverride() : 1.0f;
}

UWorld* UUTHUDWidget::GetWorld() const
{
	return (UTHUDOwner) ? UTHUDOwner->GetWorld() : nullptr;
}

FString UUTHUDWidget::GetClampedName(AUTPlayerState* PS, UFont* NameFont, float NameScale, float MaxWidth)
{
	if (!PS)
	{
		return "";
	}
	if (PS->bHasValidClampedName && (PS->NameClamper == this))
	{
		return PS->ClampedName;
	}

	PS->ClampedName = PS->PlayerName;
	PS->NameClamper = this;
	float XL = 0.f;
	float YL = 0.f;
	Canvas->TextSize(NameFont, PS->ClampedName, XL, YL, NameScale, NameScale);
	int32 ClampedLen = PS->PlayerName.Len();
	while ((XL > MaxWidth) && (ClampedLen > 1))
	{
		ClampedLen -= 1;
		PS->ClampedName = PS->PlayerName.Left(ClampedLen) + "...";
		Canvas->TextSize(NameFont, PS->ClampedName, XL, YL, NameScale, NameScale);
	}
	PS->bHasValidClampedName = true;
	return PS->ClampedName;
}

FVector2D UUTHUDWidget::CalcRotatedDrawLocation(float DistanceInPixels, float Angle)
{
	float Sin = 0.f;
	float Cos = 0.f;

	FMath::SinCos(&Sin,&Cos, FMath::DegreesToRadians(Angle));
	FVector2D NewPoint;

	NewPoint.X = DistanceInPixels * Sin;
	NewPoint.Y = -1.0f * DistanceInPixels * Cos;
	return NewPoint;
}



