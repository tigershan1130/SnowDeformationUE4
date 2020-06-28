// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowDeformCapture2D.h"
#include <SnowDeformationPlugin\Public\AddRTPixelShader.h>

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


// Called every frame
void USnowDeformCapture2D::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (SnowDeformRT == nullptr)
		return;

	if (GlobalDeformRT == nullptr)
		return;

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

		SceneCapture->AddLocalRotation(FRotator(90,90, 0));

		APlayerController* currentPlayer = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		FVector Position = currentPlayer->GetPawn()->GetActorLocation() + FVector(0, 0, -CaptureHeight - 100); // person height offset of 80

		// since ortho doesn;t matter.
		SceneCapture->SetWorldLocation(Position);
	}

	// set our scene capture to our Scene Object Transform
	SceneCapture->PostProcessSettings.AddBlendable(InBlendableObject, 1);

	// capture scene
	SceneCapture->TextureTarget = SnowDeformRT;
	//SceneCapture->CaptureScene();


	FTextureRHIRef RenderTargetInput = (SceneCapture->TextureTarget->Resource)->TextureRHI;

	FTextureRHIRef GlobalTargetInput = (GlobalDeformRT->Resource)->TextureRHI;

	UTextureRenderTarget2D* RenderTargetPersistent = GlobalDeformRT;
	
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[RenderTargetInput, GlobalTargetInput, RenderTargetPersistent](FRHICommandListImmediate& RHICmdList)
		{
			FCopyTexturePixelShader::DrawToRenderTarget_RenderThread(RHICmdList, RenderTargetInput, GlobalTargetInput, RenderTargetPersistent);
		}
		);


}

