// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowDeformCapture2D.h"
#include <SnowDeformationPlugin\Public\AddRTPixelShader.h>
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values for this component's properties
USnowDeformCapture2D::USnowDeformCapture2D()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USnowDeformCapture2D::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void USnowDeformCapture2D::BeginDestroy()
{

	Super::BeginDestroy();

	if (SnowDeformRT != nullptr)
	{
		SnowDeformRT->UpdateResourceImmediate(true);

		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), SnowDeformRT, FLinearColor(0, 0, 0, 0));
	}

	if (GlobalDeformRT != nullptr)
	{
		GlobalDeformRT->UpdateResourceImmediate(true);
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), GlobalDeformRT, FLinearColor(0, 0, 0, 0));
	}
}



// Called every frame
void USnowDeformCapture2D::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (SnowDeformRT == nullptr)
		return;

	if (GlobalDeformRT == nullptr)
		return;


	APlayerController* currentPlayer = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	FVector Position = currentPlayer->GetPawn()->GetActorLocation() + FVector(0, 0, -CaptureHeight - 100); // person height offset of 80
	FVector2D OffsetPos = FVector2D(0, 0);

	if (SceneCapture == nullptr)
	{
		ASceneCapture2D* sceneCaptureActor = (ASceneCapture2D*)GetWorld()->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass());

		SceneCapture = sceneCaptureActor->GetCaptureComponent2D();
		SceneCapture->ProjectionType = ECameraProjectionMode::Type::Orthographic;
		SceneCapture->FOVAngle = 90.0f;
		SceneCapture->OrthoWidth = CaptureSize; // for ratio would be 2048:256, so if we set our width to 1024 in RT, the detail would be 1:2 pixel ratio.
		SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		SceneCapture->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;
		SceneCapture->bCaptureOnMovement = true;
		SceneCapture->bCaptureEveryFrame = true;
		SceneCapture->MaxViewDistanceOverride = CaptureHeight;
		SceneCapture->bAutoActivate = true;
		//SceneCapture->DetailMode = EDetailMode::DM_High;

		SceneCapture->AddLocalRotation(FRotator(90, 90, 0));

		SceneCapture->SetWorldLocation(Position);

		PreviousCapturePos = Position;
	}

	float ProjectionRatio = float(SnowDeformRT->SizeX) / float(CaptureSize);

	float PixelWorldSize = (1 / float(SnowDeformRT->SizeX)) * (float)CaptureSize;
	float Dx = (floor(Position.X / PixelWorldSize) + 0.5) * PixelWorldSize;
	float Dy = (floor(Position.Y / PixelWorldSize) + 0.5) * PixelWorldSize;

	// So this will be in range of 0-1 of discrete 
	// let's look at this in detail so Postion.X and Position.Y are current position in world position it could be 0.5, 0.5
	// so the above equation would map that to 1,1 for Dx,Dy
	// then the actual discrete offset would be 1-0.5, 1-0.5
	// which would give actual offset position.
	//the Dx Dy would always calculate a value that's 0-1 discretly bigger than actual pos
	OffsetPos = FVector2D(Dx - PreviousCapturePos.X, Dy - PreviousCapturePos.Y);


	UE_LOG(LogTemp, Warning, TEXT("%f, %f : %f, %f "), Dx, Dy, PreviousCapturePos.X, PreviousCapturePos.Y);
	//if (OffsetPos.X < 1 && OffsetPos.Y < 1)
	//	return;

	// set our scene capture to our Scene Object Transform
	SceneCapture->PostProcessSettings.AddBlendable(InBlendableObject, 1);

	// Capture Scene
	SceneCapture->TextureTarget = SnowDeformRT;
	UMaterialParameterCollectionInstance* inst = GetWorld()->GetParameterCollectionInstance(MaterialParameterCollectionAsset);

	//inst->SetVectorParameterValue("CaptureLocation", FVector(0,0,0));

	FTextureRHIRef RenderTargetInput = SnowDeformRT->GameThread_GetRenderTargetResource()->TextureRHI;
	FTextureRHIRef GlobalTargetInput = GlobalDeformRT->GameThread_GetRenderTargetResource()->TextureRHI;

	UTextureRenderTarget2D* RenderTargetPersistent = GlobalDeformRT;

	FVector2D OffsetUV = OffsetPos / CaptureSize;

	FVector CaptureLocation = FVector(PreviousCapturePos.X + OffsetPos.X, OffsetPos.Y + PreviousCapturePos.Y, Position.Z);
	// since ortho doesn't matter.
	SceneCapture->SetWorldLocation(CaptureLocation);

	PreviousCapturePos = CaptureLocation;

	inst->SetVectorParameterValue("CaptureLocation", CaptureLocation);
	inst->SetScalarParameterValue("ProjectionRatio", ProjectionRatio);

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[RenderTargetInput, GlobalTargetInput, RenderTargetPersistent, OffsetUV](FRHICommandListImmediate& RHICmdList)
		{
			FCopyTexturePixelShader::DrawToRenderTarget_RenderThread(RHICmdList, RenderTargetInput, GlobalTargetInput, RenderTargetPersistent, OffsetUV);
		}
	);

	if (GlobalDeformRT != nullptr)
		GlobalDeformRT->UpdateResourceImmediate(false);

	if (SnowDeformRT != nullptr)
		SnowDeformRT->UpdateResourceImmediate(true);

	
}

