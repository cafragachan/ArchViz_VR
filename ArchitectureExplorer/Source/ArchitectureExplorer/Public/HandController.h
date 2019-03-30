// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandController.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AHandController : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHandController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	// Default sub object

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* MotionController;

	UPROPERTY(EditAnywhere)
	class UHapticFeedbackEffect_Base*  HapticEffect = nullptr;

	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	void ClimbingMovement();
	bool CanClimb() const;

	class APlayerController* PlayerController = nullptr;
	EControllerHand MyHand;
	AHandController* OtherController = nullptr;


public:

	//State
	bool bCanClimb = false;
	bool bIsClimbing = false;
	FVector ClimbingCharacterInitialPosition;
	FVector ClimbingControllerInitialPosition;

	//Setters
	void SetHand(EControllerHand Hand);
	void SetOtherController(AHandController* Controller_);
	void SetPlayerController(APlayerController* PlayerController_);

	//Input bindings
	void Grip();
	void Release();

};
