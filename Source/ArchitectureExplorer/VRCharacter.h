// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

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
	void MoveForward(float throttle);
	void MoveRight(float throttle);

	void UpdateDestinationMarker();
	bool FindTeleportDestination(TArray<FVector> &OutPath, FVector& OutLocation);

	void BeginTeleport();
	void FinishTeleport();

	void DebugPoint();
	void CameraFade(float InAlpha, float outAlpha);

	void UpdateBlinkers();
	void DrawTeleportPath(const TArray<FVector>& Path);
	void UpdateSpline(const TArray<FVector>& Path);
	
	FVector2D GetBlinkerCenter();



private:
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
	class AHandController* LeftController;

	UPROPERTY(VisibleAnywhere)
	class AHandController* RightController;

	UPROPERTY()
	class USceneComponent* VRRoot;



	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(VisibleAnywhere)
	class UPostProcessComponent* PostProcessComponent;


	UPROPERTY(EditAnywhere)
	float MaxRange = 1000.0f;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10.0f;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 1000.0f;

	UPROPERTY(EditAnywhere)
	float TeleportMaxSimTime = 2;

	UPROPERTY(EditAnywhere)
	float FadeTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float Speed= 10.0f;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	FHitResult TeleHit;

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;


	UPROPERTY(VisibleAnywhere)
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	UPROPERTY(EditDefaultsOnly)
	class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* TeleportArchMaterial;


	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY()
	float BlinkRadius = 1.0f ;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AHandController> HandControllerClass;

};
