// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUDWidget_NetInfo.h"

extern ENGINE_API float GAverageFPS;

UUTHUDWidget_NetInfo::UUTHUDWidget_NetInfo(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DesignedResolution = 1080;
	Position = FVector2D(0, 0);
	Size = FVector2D(200.0f, 400.0f);
	ScreenPosition = FVector2D(0.0f, 0.25f);
	Origin = FVector2D(0.0f, 0.0f);

	DataColumnX = 0.5f;

	ValueHighlight[0] = FLinearColor::White;
	ValueHighlight[1] = FLinearColor::Yellow;
	ValueHighlight[2] = FLinearColor::Red;
	LastPacketsIn = 200;
	LastPacketsOut = 200;
	bShouldKickBack = false;
}

bool UUTHUDWidget_NetInfo::ShouldDraw_Implementation(bool bShowScores)
{
	return (!bShowScores && UTHUDOwner && Cast<AUTPlayerController>(UTHUDOwner->PlayerOwner) && Cast<AUTPlayerController>(UTHUDOwner->PlayerOwner)->bShowNetInfo);
}

void UUTHUDWidget_NetInfo::AddPing(float NewPing)
{
	PingHistory[BucketIndex] = NewPing;
	BucketIndex++;
	if (BucketIndex > 299)
	{
		BucketIndex = 0;
	}
	NumPingsRcvd++;
}

float UUTHUDWidget_NetInfo::CalcAvgPing()
{
	if (UTHUDOwner && (UTHUDOwner->GetNetMode() == NM_Standalone))
	{
		return 0.f;
	}

	float TotalPing = 0.f;
	if (BucketIndex < 100)
	{
		// wrap around
		for (int32 i = 200 + BucketIndex; i < 300; i++)
		{
			TotalPing += PingHistory[i];
		}
	}
	for (int32 i = FMath::Max(0, BucketIndex - 100); i < BucketIndex; i++)
	{
		TotalPing += PingHistory[i];
	}
	return TotalPing/FMath::Min(100.f, float(NumPingsRcvd));
}

float UUTHUDWidget_NetInfo::CalcPingStandardDeviation(float AvgPing)
{
	float Variance = 0.f;
	MaxDeviation = 0.f;
	if (UTHUDOwner && (UTHUDOwner->GetNetMode() != NM_Standalone))
	{
		int32 NumPings = FMath::Min(100, NumPingsRcvd);
		if (BucketIndex < 100)
		{
			// wrap around
			for (int32 i = 200 + BucketIndex; i < 200 + NumPings; i++)
			{
				Variance += FMath::Square(PingHistory[i] - AvgPing);
				MaxDeviation = FMath::Max(MaxDeviation, FMath::Abs(PingHistory[i] - AvgPing));
			}
		}
		for (int32 i = FMath::Max(0, BucketIndex - NumPings); i < BucketIndex; i++)
		{
			Variance += FMath::Square(PingHistory[i] - AvgPing);
			MaxDeviation = FMath::Max(MaxDeviation, FMath::Abs(PingHistory[i] - AvgPing));
		}
		Variance = Variance / float(NumPings);
	}
	return FMath::Sqrt(Variance);
}

void UUTHUDWidget_NetInfo::Draw_Implementation(float DeltaTime)
{
	Super::Draw_Implementation(DeltaTime);

	if (!UTHUDOwner || !UTHUDOwner->PlayerOwner)
	{
		return;
	}
	float XOffset = 16.f;
	float DrawOffset = 0.f;
	AUTPlayerState* UTPS = Cast<AUTPlayerState>(UTHUDOwner->PlayerOwner->PlayerState);
	UNetDriver* NetDriver = GetWorld()->GetNetDriver();

	float XL, SmallYL;
	Canvas->TextSize(UTHUDOwner->TinyFont, "TEST", XL, SmallYL, RenderScale, RenderScale);
	DrawTexture(UTHUDOwner->ScoreboardAtlas, 0.5f*XOffset, DrawOffset, Size.X, 14*SmallYL, 149, 138, 32, 32, 0.4f, FLinearColor::Black);
	DrawOffset += 0.5f * SmallYL;

	if (UTPS)
	{
		// frame rate
		DrawText(NSLOCTEXT("NetInfo", "FrameRate title", "FPS"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		FFormatNamedArguments FPSArgs;
		FPSArgs.Add("FPS", FText::AsNumber(int32(GAverageFPS)));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "FPS", "{FPS}"), FPSArgs), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(UTPS->ExactPing, 70.f, 160.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		// ping
		DrawText(NSLOCTEXT("NetInfo", "Ping title", "Ping"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		FFormatNamedArguments Args;
		Args.Add("PingMS", FText::AsNumber(int32(UTPS->ExactPing)));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "Ping", "{PingMS} ms"), Args), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(UTPS->ExactPing, 70.f, 160.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		float AvgPing = CalcAvgPing();
		FFormatNamedArguments AltArgs;
		AltArgs.Add("PingMS", FText::AsNumber(int32(1000.f*AvgPing)));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "AltPing", "{PingMS} ms Avg"), AltArgs), XOffset + 0.5f*DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(1000.f*AvgPing, 70.f, 160.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		float PingSD = CalcPingStandardDeviation(AvgPing);
		FFormatNamedArguments SDArgs;
		SDArgs.Add("PingSD", FText::AsNumber(int32(1000.f*PingSD)));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "Ping Standard Deviation", "{PingSD} StdDev"), SDArgs), XOffset + 0.5f*DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(1000.f*PingSD, 20.f, 40.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		FFormatNamedArguments MaxArgs;
		MaxArgs.Add("MaxD", FText::AsNumber(int32(1000.f*MaxDeviation)));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "Ping Max Deviation", "{MaxD} MaxDev"), MaxArgs), XOffset + 0.5f*DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(1000.f*MaxDeviation, 35.f, 60.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		DrawOffset += SmallYL;
	}

	// netspeed
	if (UTHUDOwner->PlayerOwner->Player)
	{
		DrawText(NSLOCTEXT("NetInfo", "NetSpeed title", "Net Speed"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		FFormatNamedArguments Args;
		Args.Add("NetBytes", FText::AsNumber(UTHUDOwner->PlayerOwner->Player->CurrentNetSpeed));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "NetBytes", "{NetBytes} bytes/s"), Args), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;
	}

	// bytes in and out, and packet loss
	if (NetDriver)
	{
		if (int32(NetDriver->InPackets) < LastPacketsIn)
		{
			PacketsIn = LastPacketsIn;
		}
		if (int32(NetDriver->OutPackets) < LastPacketsOut)
		{
			PacketsOut = LastPacketsOut;
		}
		LastPacketsIn = NetDriver->InPackets;
		LastPacketsOut = NetDriver->OutPackets;

		DrawText(NSLOCTEXT("NetInfo", "PacketsINtitle", "Pkts In"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		FFormatNamedArguments Args;
		Args.Add("PacketsIN", FText::AsNumber(PacketsIn));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "NetPacketsIN", "{PacketsIN}/sec"), Args), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		DrawText(NSLOCTEXT("NetInfo", "PacketsOUTtitle", "Pkts Out"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		Args.Add("PacketsOUT", FText::AsNumber(PacketsOut));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "NetPacketsOUT", "{PacketsOUT}/sec"), Args), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		DrawText(NSLOCTEXT("NetInfo", "BytesIn title", "Bytes In"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		Args.Add("NetBytesIN", FText::AsNumber(NetDriver->InBytesPerSecond));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "NetBytesIN", "{NetBytesIN} Bps"), Args), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		DrawText(NSLOCTEXT("NetInfo", "BytesOut title", "Bytes Out"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		Args.Add("NetBytes", FText::AsNumber(NetDriver->OutBytesPerSecond));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "NetBytes", "{NetBytes} Bps"), Args), XOffset + DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;
		DrawOffset += SmallYL;

		DrawText(NSLOCTEXT("NetInfo", "PLOUT title", "Pkt Loss Out"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		Args.Add("PLOUT", FText::AsNumber(NetDriver->OutPacketsLost));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "PLOUT", "{PLOUT}%"), Args), XOffset + 1.1f*DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(NetDriver->OutPacketsLost, 0.1f, 1.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;

		DrawText(NSLOCTEXT("NetInfo", "PLIN title", "Pkt Loss In"), XOffset, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		Args.Add("PLIN", FText::AsNumber(NetDriver->InPacketsLost));
		DrawText(FText::Format(NSLOCTEXT("NetInfo", "PLIN", "{PLIN}%"), Args), XOffset + 1.1f*DataColumnX*Size.X, DrawOffset, UTHUDOwner->TinyFont, 1.0f, 1.0f, GetValueColor(NetDriver->InPacketsLost, 0.1f, 1.f), ETextHorzPos::Left, ETextVertPos::Center);
		DrawOffset += SmallYL;
	}

	// packet loss - also measure burstiness

	// ping prediction stats
}

FLinearColor UUTHUDWidget_NetInfo::GetValueColor(float Value, float ThresholdBest, float ThresholdWorst) const
{
	if (Value < ThresholdBest)
	{
		return ValueHighlight[0];
	}
	if (Value > ThresholdWorst)
	{
		return ValueHighlight[2];
	}
	return ValueHighlight[1];
}

