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
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HeadMountedDisplay/Public/MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(FName("VR_Root"));
	VRRoot->SetupAttachment(GetRootComponent());

	VRCamera = CreateDefaultSubobject<UCameraComponent>(FName("VR_Camera"));
	VRCamera->SetupAttachment(VRRoot);

	LeftControllerTouch = CreateDefaultSubobject<UMotionControllerComponent>(FName("LeftController"));
	LeftControllerTouch->SetupAttachment(VRRoot);
	LeftControllerTouch->SetTrackingSource(EControllerHand::Left);

	RightControllerTouch = CreateDefaultSubobject<UMotionControllerComponent>(FName("RightController"));
	RightControllerTouch->SetupAttachment(VRRoot);
	RightControllerTouch->SetTrackingSource(EControllerHand::Right);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(FName("VR_Marker"));
	DestinationMarker->SetupAttachment(VRRoot);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(FName("TeleportPath"));
	TeleportPath->SetupAttachment(RightControllerTouch);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(FName("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	PlayerCameraManager = PC->PlayerCameraManager;
	BlinkerInstance = UMaterialInstanceDynamic::Create(BlinkerMaterial, this);

	
}


// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraCorrection();
	//UpdateMarkerLocation();
	ProjectileMarker();
	UpdateBlinker();
	
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
	//SetBlinkerMaterial(Forward_); //use this as linear not as curve
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

void AVRCharacter::UpdateBlinker()
{
	if (!ensure(BlinkerInstance && Curve)) return;
	//BlinkerInstance->SetScalarParameterValue("BlinkerRadius", 1.5 - FMath::Abs(Value_)); /////// use this for linear interpolation

	auto Radius = Curve->GetFloatValue(GetVelocity().Size());
	BlinkerInstance->SetScalarParameterValue("BlinkerRadius", Radius);

	auto Center = GetBlinkerCenter();
	BlinkerInstance->SetVectorParameterValue("Center", FLinearColor(Center.X, Center.Y, 0));

	PostProcessComponent->AddOrUpdateBlendable(BlinkerInstance);

}

void AVRCharacter::ProjectileMarker()
{
	FPredictProjectilePathParams Params = FPredictProjectilePathParams
	(
		5.f,
		RightControllerTouch->GetComponentLocation(),
		RightControllerTouch->GetForwardVector() * ProjectilePathVelocity,
		1.f,
		ECollisionChannel::ECC_Visibility,
		GetOwner()
	);

	//Params.DrawDebugType = EDrawDebugTrace::ForOneFrame; // draw debug spheres
	Params.bTraceComplex = true;

	FPredictProjectilePathResult PathResult;

	bool bPath = UGameplayStatics::PredictProjectilePath(GetWorld(), Params, PathResult);

	if (bPath)
	{
		//DrawSpline
		TeleportPath->ClearSplinePoints();

		auto PathResultData = PathResult.PathData;

		PointsInSpace.Empty();

		for(int i = 0; i < PathResultData.Num(); ++i)
		{
			PointsInSpace.Add(PathResultData[i].Location);

			if (i >= PathResultData.Num() - 1) continue;

			if (SplineMeshPool.Num() <= i)
			{
				USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
				SplineMesh->SetMobility(EComponentMobility::Movable);
				SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
				SplineMesh->SetStaticMesh(TeleportArchMesh);
				SplineMesh->SetMaterial(0, TeleportArchMaterial);
				SplineMesh->RegisterComponent();
				SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				SplineMeshPool.Add(SplineMesh);
			}

			SplineMeshPool[i]->SetWorldLocation(PathResultData[i].Location);
			SplineMeshPool[i]->SetVisibility(true);


			if (i < PathResultData.Num() - 1)
			{
				auto LocalPos = SplineMeshPool[i]->GetComponentTransform().InverseTransformPosition(PathResultData[i + 1].Location);
				SplineMeshPool[i]->SetEndPosition(LocalPos);
			}
		}

		TeleportPath->SetSplinePoints(PointsInSpace, ESplineCoordinateSpace::World, true);

		for (int i = 0; i < SplineMeshPool.Num(); ++i)
		{
			auto StartTangent = TeleportPath->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
			auto EndTangent = TeleportPath->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);

			SplineMeshPool[i]->SetStartTangent(StartTangent);
			SplineMeshPool[i]->SetEndTangent(EndTangent);
		}

		//Update Marker
		DestinationMarker->SetVisibility(true);
		auto NavMesh = UNavigationSystemV1::GetNavigationSystem(GetWorld());
		FNavLocation NavPos;
		bool bNewPos = NavMesh->ProjectPointToNavigation(PathResult.HitResult.Location, NavPos, FVector(100, 100, 100));

		if (bNewPos)
		{
			DestinationMarker->SetWorldLocation(NavPos);
		}

		else
		{
			DestinationMarker->SetVisibility(false);

			if (SplineMeshPool.Num() > 0)
			{
				for (auto& Spline : SplineMeshPool)
				{
					Spline->SetVisibility(false);
				}
			}
		}

	}
	else
	{
		DestinationMarker->SetVisibility(false);

		if (SplineMeshPool.Num() > 0)
		{
			for(auto& Spline : SplineMeshPool)
			{
				Spline->SetVisibility(false);
			}
		}
	}
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	if (GetVelocity().IsNearlyZero())	return FVector2D(0.5, 0.5);

	auto VelDirection = GetVelocity().GetSafeNormal();
	FVector FuturePosition;

	if (FVector::DotProduct(VelDirection, VRCamera->GetForwardVector()) > 0)
	{
		FuturePosition = GetVelocity().GetSafeNormal() * 1000 + VRCamera->GetComponentLocation();
	}
	else
	{
		FuturePosition = - GetVelocity().GetSafeNormal() * 1000 + VRCamera->GetComponentLocation();
	}

	FVector2D ScreenPosition;
	auto PC = Cast<APlayerController>(GetController());
	PC->ProjectWorldLocationToScreen(FuturePosition, ScreenPosition);
		
	//Mapped Values 
	int32 XVal = (int32)ScreenPosition.X;
	int32 YVal = (int32)ScreenPosition.Y;

	int32 VPX, VPY;
	PC->GetViewportSize(VPX, VPY);

	FVector2D Range = FVector2D(0, 1);
	FVector2D VPSizeRangeX = FVector2D(0, VPX);
	FVector2D VPSizeRangeY = FVector2D(0, VPY);

	auto XMapped = FMath::GetMappedRangeValueClamped(VPSizeRangeX, Range, XVal);
	auto YMapped = FMath::GetMappedRangeValueClamped(VPSizeRangeY, Range, YVal);

	return FVector2D(XMapped, YMapped);

}


void AVRCharacter::UpdateMarkerLocation()
{
	
}

