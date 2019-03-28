// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"


class UCameraComponent;
class APlayerCameraManager;

UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void UpdateMarkerLocation();

	void CameraCorrection();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* VRCamera = nullptr;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* VRRoot = nullptr;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* DestinationMarker = nullptr;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float LineTraceReach = 1000;

	APlayerCameraManager* PlayerCameraManager = nullptr;

	void CharacterForwardMovement(float Forward_);
	void CharacterRightMovement(float Right_);
	void BeginTeletransport();
	void EndTeletransport();
};
