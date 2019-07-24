#include "UnrealTournament.h"
#include "UTProj_BioGrenade.h"
#include "UnrealNetwork.h"

AUTProj_BioGrenade::AUTProj_BioGrenade(const FObjectInitializer& OI)
	: Super(OI)
{
	ProximitySphere = OI.CreateOptionalDefaultSubobject<USphereComponent>(this, TEXT("ProxSphere"));
	if (ProximitySphere != NULL)
	{
		ProximitySphere->InitSphereRadius(200.0f);
		ProximitySphere->SetCollisionObjectType(COLLISION_PROJECTILE);
		ProximitySphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ProximitySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		ProximitySphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		ProximitySphere->OnComponentBeginOverlap.AddDynamic(this, &AUTProj_BioGrenade::ProximityOverlapBegin);
		ProximitySphere->bTraceComplexOnMove = false;
		ProximitySphere->bReceivesDecals = false;
		ProximitySphere->SetupAttachment(RootComponent);
	}

	TimeBeforeFuseStarts = 2.0f;
	FuseTime = 1.0f;
	ProximityDelay = 0.1f;
}

void AUTProj_BioGrenade::BeginPlay()
{
	Super::BeginPlay();

	SetTimerUFunc(this, FName(TEXT("StartFuseTimed")), TimeBeforeFuseStarts, false);
}

void AUTProj_BioGrenade::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	if (Instigator != nullptr)
	{
		static FName NAME_TeamColor(TEXT("TeamColor"));
		TArray<UParticleSystemComponent*> PSCs;
		GetComponents<UParticleSystemComponent>(PSCs);
		FVector TeamColor = FVector(0.7f, 0.4f, 0.f);
		AUTCharacter* UTChar = Cast<AUTCharacter>(Instigator);
		if (UTChar)
		{
			FLinearColor LinearTeamColor = UTChar->GetTeamColor();
			TeamColor = FVector(LinearTeamColor.R, LinearTeamColor.G, LinearTeamColor.B);
		}
		for (int32 i=0; i<PSCs.Num(); i++)
		{
			if (PSCs[i] != nullptr)
			{
				PSCs[i]->SetColorParameter(NAME_TeamColor, TeamColor);
			}
		}
	}
}

void AUTProj_BioGrenade::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTProj_BioGrenade, bBeginFuseWarning);
}

void AUTProj_BioGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bBeginFuseWarning && GetWorldTimerManager().IsTimerActive(FuseTimerHandle))
	{
		FRotator NewRotation(0.f, 0.f, 0.f);
		NewRotation.Yaw = GetActorRotation().Yaw + 100.f*DeltaTime + 2000.f*(FuseTime - GetWorldTimerManager().GetTimerRemaining(FuseTimerHandle))*DeltaTime;
		SetActorRotation(NewRotation);
	}
	if (GetVelocity().IsNearlyZero() && !FlightAudioComponent)
	{
		// deactivate flight audio
		TArray<UAudioComponent*> SCs;
		GetComponents<UAudioComponent>(SCs);
		FlightAudioComponent = SCs.Num() > 0 ? SCs[0] : nullptr;
		if (FlightAudioComponent)
		{
			FlightAudioComponent->Deactivate();
		}
	}
}

void AUTProj_BioGrenade::StartFuseTimed()
{
	if (!bBeginFuseWarning)
	{
		StartFuse();
	}
}
void AUTProj_BioGrenade::StartFuse()
{
	bBeginFuseWarning = true;
	
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AUTProj_BioGrenade::FuseExpired, FuseTime, false);
	PlayFuseBeep();
	ClearTimerUFunc(this, FName(TEXT("StartFuseTimed")));
}

void AUTProj_BioGrenade::FuseExpired()
{
	if (!bExploded)
	{
		Explode(GetActorLocation(), FVector(0.0f, 0.0f, 1.0f));
	}
}

void AUTProj_BioGrenade::PlayFuseBeep()
{
	if (!bExploded)
	{
		UUTGameplayStatics::UTPlaySound(GetWorld(), FuseBeepSound, this, SRT_IfSourceNotReplicated, false, FVector::ZeroVector, NULL, NULL, true, SAT_WeaponFoley);
		if (FuseEffect != NULL && GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			UParticleSystemComponent* FusePSC = UGameplayStatics::SpawnEmitterAttached(FuseEffect, RootComponent);
			if (FusePSC != nullptr)
			{
				static FName NAME_TeamColor(TEXT("TeamColor"));
				FVector TeamColor = FVector(0.7f, 0.4f, 0.f);
				AUTCharacter* UTChar = Cast<AUTCharacter>(Instigator);
				if (UTChar)
				{
					FLinearColor LinearTeamColor = UTChar->GetTeamColor();
					TeamColor = FVector(LinearTeamColor.R, LinearTeamColor.G, LinearTeamColor.B);
				}
				FusePSC->SetColorParameter(NAME_TeamColor, TeamColor);
			}
		}
		// if there's enough fuse time left queue another beep
		float TotalTime, ElapsedTime;
		if (IsTimerActiveUFunc(this, FName(TEXT("FuseExpired")), &TotalTime, &ElapsedTime) && TotalTime - ElapsedTime > 0.5f)
		{
			SetTimerUFunc(this, FName(TEXT("PlayFuseBeep")), 0.5f, false);
		}
	}
}

void AUTProj_BioGrenade::OnStop(const FHitResult& Hit)
{
	// intentionally blank
	SetActorLocation(GetActorLocation() + 5.f * Hit.ImpactNormal);
}

void AUTProj_BioGrenade::ProximityOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != NULL && OtherActor != Instigator && !bBeginFuseWarning && GetWorld()->TimeSeconds - CreationTime >= ProximityDelay)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS == NULL || !GS->OnSameTeam(OtherActor, InstigatorController))
		{
			FVector OtherLocation;
			if (bFromSweep)
			{
				OtherLocation = SweepResult.Location;
			}
			else
			{
				OtherLocation = OtherActor->GetActorLocation();
			}

			FCollisionQueryParams Params(FName(TEXT("PawnSphereOverlapTrace")), true, this);
			Params.AddIgnoredActor(OtherActor);

			// since PawnOverlapSphere doesn't hit blocking objects, it is possible it is touching a target through a wall
			// make sure that the hit is valid before proceeding
			if (!GetWorld()->LineTraceTestByChannel(OtherLocation, GetActorLocation(), COLLISION_TRACE_WEAPON, Params))
			{
				StartFuse();
			}
		}
	}
}