// Fill out your copyright notice in the Description page of Project Settings.

#include "HandController.h"
#include "HeadMountedDisplay/Public/MotionControllerComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AHandController::AHandController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);
}


// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHandController::SetHand(EControllerHand Hand)
{
	MotionController->SetTrackingSource(Hand);

	if (Hand == EControllerHand::Right)
	{
		auto Mesh = FindComponentByClass<UStaticMeshComponent>();
		auto Mat = Mesh->GetMaterial(0);
		if(Mesh) Mesh->SetRelativeScale3D(FVector(1, -1, 1));

		Mesh->SetMaterial(0, Mat);
	}
}
