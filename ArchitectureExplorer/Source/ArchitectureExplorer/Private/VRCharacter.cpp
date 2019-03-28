// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"
#include "Classes/Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Public/DrawDebugHelpers.h"
#include "Camera/PlayerCameraManager.h"
#include "Public/TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"

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
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	PlayerCameraManager = PC->PlayerCameraManager;
	
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
	PlayerInputComponent->BindAction("Teleport", EInputEvent::IE_Pressed, this, &AVRCharacter::BeginTeletransport);
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

void AVRCharacter::BeginTeletransport()
{

	if (!ensure(PlayerCameraManager)) return;

	PlayerCameraManager->StartCameraFade(0.f, 1.f, 1.f, FLinearColor::Black);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AVRCharacter::EndTeletransport, 1.f, false);
}

void AVRCharacter::EndTeletransport()
{

	auto HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(DestinationMarker->GetComponentLocation() + (FVector::UpVector * HalfHeight));
	PlayerCameraManager->StartCameraFade(1.f, 0.f, 1, FLinearColor::Black);

}

void AVRCharacter::UpdateMarkerLocation()
{
	FHitResult Hit;
	FVector LineEnd = VRCamera->GetForwardVector() * LineTraceReach + VRCamera->GetComponentLocation();
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, VRCamera->GetComponentLocation(), LineEnd, ECollisionChannel::ECC_Visibility);

	if (bHit && Hit.Normal.Z > 0.9)
	{
		DestinationMarker->SetVisibility(true);
		auto RayPoint = Hit.Location;
		DestinationMarker->SetWorldLocation(RayPoint);
	}
	else
	{
		DestinationMarker->SetVisibility(false);

	}
}


