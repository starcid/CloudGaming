// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealTournament.h"
#include "UTFocusTracerComponent.h"
#include "../ThirdParty/CloudImp/FocusTrace/FocusTraceSystem.h"
#include "../ThirdParty/CloudImp/Implement/UE4/UTFocusTracer.h"

// Sets default values for this component's properties
UUTFocusTracerComponent::UUTFocusTracerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	tracer = new UTFocusTracer();
}

UUTFocusTracerComponent::~UUTFocusTracerComponent()
{
	if (tracer != NULL)
		delete tracer;
}

// Called when the game starts
void UUTFocusTracerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	tracer->Initialize(GetOwner(), Priority, Key);
	FocusTraceSystem::Instance()->Register(tracer);
}

void UUTFocusTracerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FocusTraceSystem::Instance()->UnRegister(tracer);
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void UUTFocusTracerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UUTFocusTracerComponent::SetTracerEnable(bool enable)
{ 
	tracer->SetEnable(enable); 
}

