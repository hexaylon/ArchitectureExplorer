// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/Vector.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "HandController.h"
#include "XRMotionControllerBase.h"


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);


	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);
	if (BlinkerMaterialBase != nullptr) 
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
		
		
	}

	LeftController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftController != nullptr)
	{

		LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->SetOwner(this);
		LeftController->SetHand(EControllerHand::Left);
	}

	RightController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightController != nullptr)
	{

		RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightController->SetOwner(this);
		RightController->SetHand(EControllerHand::Right);
	}

}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	UpdateDestinationMarker();
	UpdateBlinkers();

	
}


bool AVRCharacter::FindTeleportDestination(TArray<FVector>& OutPath, FVector& OutLocation)
{

	if (LeftController == nullptr)
	{
		return true;
	}

	FVector Start = LeftController->GetActorLocation();
	FVector Look = LeftController->GetActorForwardVector();
	Look = Look.RotateAngleAxis(30, LeftController->GetActorRightVector());

	FVector End = Start + Look * MaxRange;


	//DrawDebugLine(GetWorld(), Location, End, FColor::Red, false, 2.0f);

	//bool bHit = GetWorld()->LineTraceSingleByChannel(TeleHit, Start, End, ECC_Visibility);

	FPredictProjectilePathParams ProjectileParams(
		TeleportProjectileRadius,
		Start, 
		Look * TeleportProjectileSpeed,
		TeleportMaxSimTime,
		ECollisionChannel::ECC_Visibility,
		this);

	ProjectileParams.bTraceComplex = true;

	FPredictProjectilePathResult ProjectileResult;

	bool bHit = UGameplayStatics::PredictProjectilePath(this, ProjectileParams, ProjectileResult);

	if (!bHit)
	{
		return false;
	}

	//adding point from the projectile path to array based on the spline points
	for (FPredictProjectilePathPointData PointData : ProjectileResult.PathData)
	{
		OutPath.Add(PointData.Location);

	}


	FNavLocation NavLocation;

	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(ProjectileResult.HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (!bOnNavMesh)
	{
		return false;
	}

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

		/*DrawDebugPoint(GetWorld(), TeleHit.Location, 20, FColor::Red, false, 1.5F);*/
		DestinationMarker->SetWorldLocation(Location);

		DrawTeleportPath(Path);

	}
	else
	{
		DestinationMarker->SetVisibility(false);

		TArray<FVector> EmptyPath; //if not hitting, then don't draw a teleport path

		DrawTeleportPath(EmptyPath);
	}

}


void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr)
	{
		return;
	}
	float BSpeed = GetVelocity().Size();

	BlinkRadius = RadiusVsVelocity->GetFloatValue(BSpeed);

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius_Param"), BlinkRadius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Center_Param"), FLinearColor(Center.X, Center.Y, 0));
}



void AVRCharacter::DrawTeleportPath(const TArray<FVector>& Path)
{
	UpdateSpline(Path);

	for (USplineMeshComponent* SplineMesh : TeleportPathMeshPool)
	{
		SplineMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; ++i)
	{
		if (TeleportPathMeshPool.Num() <= i)
		{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent(); //importent to avoid weird stuff
			TeleportPathMeshPool.Add(SplineMesh);
		}

		USplineMeshComponent* SplineMesh = TeleportPathMeshPool[i];
		SplineMesh->SetVisibility(true);

		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i+1, EndPos, EndTangent);
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true);

	}

}



void AVRCharacter::UpdateSpline(const TArray<FVector>& Path)
{
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); ++i)
	{
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);      //inverse to local transform instead of world transform
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}

	TeleportPath->UpdateSpline();


}



FVector2D AVRCharacter::GetBlinkerCenter()
{	
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0 ) //if moving forward 
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;

	}
	else     //if moving backward 
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		return FVector2D(0.5, 0.5);

	}

	FVector2D ScreenStationaryLocation;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;

	return ScreenStationaryLocation;
}


// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"),EInputEvent::IE_Pressed,this, &AVRCharacter::BeginTeleport); 
}

void AVRCharacter::MoveForward(float throttle)
{

	AddMovementInput(throttle * Camera->GetForwardVector(), Speed);


}
void AVRCharacter::MoveRight(float throttle)
{

	AddMovementInput(throttle * Camera->GetRightVector(), Speed);
}

void AVRCharacter::BeginTeleport()
{
	CameraFade(0, 1);

	FTimerHandle VRTime;
	GetWorldTimerManager().SetTimer(VRTime, this, &AVRCharacter::FinishTeleport, FadeTime);

}

void AVRCharacter::FinishTeleport()
{	
	
	//FVector HalfHeight(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	//SetActorLocation(DestinationMarker->GetComponentLocation()+ HalfHeight);

	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);

	CameraFade(1, 0);
}

void AVRCharacter::DebugPoint()
{
	DrawDebugPoint(GetWorld(), TeleHit.Location, 50, FColor::Red, false, 1.5F);
}

void AVRCharacter::CameraFade(float InAlpha, float outAlpha)
{
	APlayerController* VRPC = Cast<APlayerController>(GetController());
	if (VRPC != nullptr)
	{
		VRPC->PlayerCameraManager->StartCameraFade(InAlpha, outAlpha, FadeTime, FLinearColor::Black);
	}
}