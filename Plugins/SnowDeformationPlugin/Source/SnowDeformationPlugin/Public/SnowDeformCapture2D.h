// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/ActorComponent.h"
#include "Engine/SceneCapture2D.h"
#include <Runtime\Engine\Public\ImageUtils.h>
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include <Runtime\Engine\Classes\Components\SceneCaptureComponent2D.h>
#include "SnowDeformCapture2D.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNOWDEFORMATIONPLUGIN_API USnowDeformCapture2D : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USnowDeformCapture2D();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Deform Capture")
	UTextureRenderTarget2D* SnowDeformRT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Deform Capture")
	UTextureRenderTarget2D* GlobalDeformRT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Deform Capture")
	TScriptInterface<IBlendableInterface> InBlendableObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Deform Capture")
		float CaptureSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snow Deform Capture")
		float CaptureHeight;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	USceneCaptureComponent2D* SceneCapture = nullptr;
	
	// copy 不行， 得Add 像素点到Shader 里
	void CopyRT_RenderThread(UTextureRenderTarget2D* m_sourceRT, UTextureRenderTarget2D* m_destRT)
	{
		check(IsInRenderingThread());
		// Get global RHI command list 
		FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();
		if (m_sourceRT && m_destRT)
		{
			FTexture2DRHIRef destTexture = m_destRT->GetRenderTargetResource()->GetRenderTargetTexture();
			FTexture2DRHIRef sourceTexture = m_sourceRT->GetRenderTargetResource()->GetRenderTargetTexture();

			FResolveParams param;
			RHICmdList.CopyToResolveTarget(sourceTexture, destTexture, param);
		}
	}
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
