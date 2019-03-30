// Fill out your copyright notice in the Description page of Project Settings.

#include "HandController.h"
#include "HeadMountedDisplay/Public/MotionControllerComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Classes/GameFramework/PlayerController.h"
#include "Classes/Haptics/HapticFeedbackEffect_Base.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);


}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ClimbingMovement();
}

void AHandController::ActorBeginOverlap(AActor * OverlappedActor, AActor * OtherActor)
{
	bool bCanClimbTemp = CanClimb();

	if(!bCanClimb & bCanClimbTemp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can Climb.."));

		if (HapticEffect && PlayerController)
		{
			PlayerController->PlayHapticEffect(HapticEffect, MyHand);
		}
	}
	bCanClimb = bCanClimbTemp;
}

void AHandController::ActorEndOverlap(AActor * OverlappedActor, AActor * OtherActor)
{
	bCanClimb = CanClimb();
	bIsClimbing = false;
}

void AHandController::ClimbingMovement()
{
	if (bIsClimbing == false) return;

	auto RelativeDistace = (GetActorLocation() - ClimbingControllerInitialPosition) * -1;
	GetAttachParentActor()->AddActorWorldOffset(RelativeDistace);
}

bool AHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);

	for (auto& OverlapActor : OverlappingActors)
	{
		if (OverlapActor->ActorHasTag("Climbable"))
		{
			return true;
		}
	}

	return false;
}

void AHandController::SetHand(EControllerHand Hand)
{
	MotionController->SetTrackingSource(Hand);
	MyHand = Hand;

	if (Hand == EControllerHand::Right)
	{
		auto Mesh = FindComponentByClass<UStaticMeshComponent>();
		auto Mat = Mesh->GetMaterial(0);
		if(Mesh) Mesh->SetRelativeScale3D(FVector(1, -1, 1));

		Mesh->SetMaterial(0, Mat);
	}
}

void AHandController::SetOtherController(AHandController * Controller_)
{
	OtherController = Controller_;
}

void AHandController::SetPlayerController(APlayerController* PlayerController_)
{
	PlayerController = PlayerController_;
}

void AHandController::Grip()
{
	if (bCanClimb == false) return;

	if (bIsClimbing == false)
	{
		OtherController->bIsClimbing = false;
		bIsClimbing = true;
		ClimbingControllerInitialPosition = GetActorLocation();

		ACharacter* Parent = Cast<ACharacter>(GetAttachParentActor());
		Parent->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	}
}

void AHandController::Release()
{
	bIsClimbing = false;

	if (OtherController->bIsClimbing == true) return;

	ACharacter* Parent = Cast<ACharacter>(GetAttachParentActor());
	Parent->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);

}
