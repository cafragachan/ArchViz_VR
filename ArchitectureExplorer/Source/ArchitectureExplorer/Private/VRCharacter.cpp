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
#include "HandController.h"
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

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(FName("VR_Marker"));
	DestinationMarker->SetupAttachment(VRRoot);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(FName("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

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

	LeftControllerTouch = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftControllerTouch != nullptr)
	{
		LeftControllerTouch->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftControllerTouch->SetHand(EControllerHand::Left);
	}

	RightControllerTouch = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightControllerTouch != nullptr)
	{
		RightControllerTouch->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightControllerTouch->SetHand(EControllerHand::Right);
	}

	
	
}


// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraCorrection();
	UpdateDestinationMarker();
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

bool AVRCharacter::FindTeleportDestination(TArray<FVector> &OutPath, FVector &OutLocation)
{
	FVector Start = RightControllerTouch->GetActorLocation();
	FVector Look = RightControllerTouch->GetActorForwardVector();

	FPredictProjectilePathParams Params(
		5,
		Start,
		Look * ProjectilePathVelocity,
		1,
		ECollisionChannel::ECC_Visibility,
		this
	);
	Params.bTraceComplex = true;
	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (!bHit) return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData)
	{
		OutPath.Add(PointData.Location);
	}

	auto NavMesh = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	FNavLocation NavLocation;
	bool bOnNavMesh = NavMesh->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, FVector(100,100,100));

	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Path, Location);

	if (bHasDestination)
	{
		DestinationMarker->SetVisibility(true);

		DestinationMarker->SetWorldLocation(Location);

		DrawTeleportPath(Path);
	}
	else
	{
		DestinationMarker->SetVisibility(false);

		TArray<FVector> EmptyPath;
		DrawTeleportPath(EmptyPath);
	}
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector> &Path)
{
	UpdateSpline(Path);

	for (USplineMeshComponent* SplineMesh : SplineMeshPool)
	{
		SplineMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; ++i)
	{
		if (SplineMeshPool.Num() <= i)
		{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent();

			SplineMeshPool.Add(SplineMesh);
		}

		USplineMeshComponent* SplineMesh = SplineMeshPool[i];
		SplineMesh->SetVisibility(true);

		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent);
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
	}
}

void AVRCharacter::UpdateSpline(const TArray<FVector> &Path)
{
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); ++i)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline();
}
