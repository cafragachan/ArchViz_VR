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

	UPROPERTY(EditAnywhere, Category = "Setup")
	float ProjectilePathVelocity = 1000;

	UPROPERTY(VisibleAnywhere)
	class UPostProcessComponent* PostProcessComponent = nullptr;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface * BlinkerMaterial = nullptr;

	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UStaticMesh * TeleportArchMesh = nullptr;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface * TeleportArchMaterial = nullptr;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* Curve = nullptr;

	UPROPERTY()
	class AHandController* LeftControllerTouch = nullptr;

	UPROPERTY()
	class AHandController* RightControllerTouch = nullptr;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AHandController> HandControllerClass;


	UPROPERTY(VisibleAnywhere)
	TArray<class USplineMeshComponent*> SplineMeshPool;

	TArray<FVector> PointsInSpace;

	APlayerCameraManager* PlayerCameraManager = nullptr;

	UMaterialInstanceDynamic* BlinkerInstance = nullptr;

	FVector2D GetBlinkerCenter();

	bool FindTeleportDestination(TArray<FVector>& OutPath, FVector & OutLocation);
	void UpdateDestinationMarker();
	void DrawTeleportPath(const TArray<FVector>& Path);
	void UpdateSpline(const TArray<FVector>& Path);
	void CharacterForwardMovement(float Forward_);
	void CharacterRightMovement(float Right_);
	void BeginTeletransport();
	void EndTeletransport();
	void UpdateBlinker();
	void CameraCorrection();

	void LeftGrip();
	void LeftRelease();
	void RightGrip();
	void RightRelease();

};
