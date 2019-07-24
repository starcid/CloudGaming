// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "UTFocusTracerComponent.generated.h"

class UTFocusTracer;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNREALTOURNAMENT_API UUTFocusTracerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUTFocusTracerComponent();

	UPROPERTY(EditAnywhere, Category = "Tracer Priority", meta = (ToolTip = "Set priority to distinguish tracer type.(Dynamic 255, UI 254, Texture complexity level 0 - 253)"))
	uint8 Priority;

	UPROPERTY(EditAnywhere, Category = "Primitive Component Key", meta = (ToolTip = "Set key words for primitive range you interest."))
	FString    Key;

	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UTFocusTracer* tracer;
	
};
