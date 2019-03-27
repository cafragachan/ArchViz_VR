// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Classes/Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Public/DrawDebugHelpers.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(FName("VR_Root"));
	VRRoot->SetupAttachment(GetRootComponent());

	VRCamera = CreateDefaultSubobject<UCameraComponent>(FName("VR_Camera"));
	VRCamera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(FName("VR_Marker"));
	DestinationMarker->SetupAttachment(VRRoot);
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraCorrection();
	UpdateMarkerLocation();

}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Forward", this, &AVRCharacter::CharacterForwardMovement);
	PlayerInputComponent->BindAxis("Right", this, &AVRCharacter::CharacterRightMovement);

}

void AVRCharacter::CameraCorrection()
{
	//Camera Position Correction
	auto CameraOffset = GetRootComponent()->GetComponentLocation() - VRCamera->GetComponentLocation();
	FVector NewPosition = VRRoot->GetComponentLocation() + CameraOffset;
	VRRoot->SetWorldLocation(NewPosition);
}

void AVRCharacter::CharacterForwardMovement(float Forward_)
{
	AddMovementInput(VRCamera->GetForwardVector(), Forward_);
}

void AVRCharacter::CharacterRightMovement(float Right_)
{
	AddMovementInput(VRCamera->GetRightVector(), Right_);

}

void AVRCharacter::UpdateMarkerLocation()
{
	FHitResult Hit;
	FVector LineEnd = VRCamera->GetForwardVector() * LineTraceReach;
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, VRCamera->GetComponentLocation(), LineEnd, ECollisionChannel::ECC_Visibility);

	if (bHit)
	{
		auto RayPoint = Hit.Location;
		DestinationMarker->SetWorldLocation(RayPoint);
	}
}


