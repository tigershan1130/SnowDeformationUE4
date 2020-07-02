﻿#include "AddRTPixelShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterMacros.h"
#include "ShaderParameterStruct.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "Runtime/RenderCore/Public/PixelShaderUtils.h"

/************************************************************************/
/* Simple static vertex buffer.                                         */
/************************************************************************/
class FSimpleScreenVertexBuffer : public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI()
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4(1, 1, 0, 1);
		Vertices[0].UV = FVector2D(1, 1);

		Vertices[1].Position = FVector4(-1, 1, 0, 1);
		Vertices[1].UV = FVector2D(0, 1);

		Vertices[2].Position = FVector4(1, -1, 0, 1);
		Vertices[2].UV = FVector2D(1, 0);

		Vertices[3].Position = FVector4(-1, -1, 0, 1);
		Vertices[3].UV = FVector2D(0, 0);



		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
TGlobalResource<FSimpleScreenVertexBuffer> GSimpleScreenVertexBuffer;

/************************************************************************/
/* A simple passthrough vertexshader that we will use.                  */
/************************************************************************/
class FSimplePassThroughVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FSimplePassThroughVS);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FSimplePassThroughVS() { }
	FSimplePassThroughVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) { }
};

/**********************************************************************************************/
/* This class carries our parameter declarations and acts as the bridge between cpp and HLSL. */
/**********************************************************************************************/
class FPixelShaderCopyTexturePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FPixelShaderCopyTexturePS);
	SHADER_USE_PARAMETER_STRUCT(FPixelShaderCopyTexturePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Texture2D, RenderTargetInput)
		SHADER_PARAMETER_SRV(Texture2D, PersistentRTInput)
		SHADER_PARAMETER(FVector2D, UVOffset)
	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

// This will tell the engine to create the shader and where the shader entry point is.
//                           ShaderType                            ShaderPath                Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FSimplePassThroughVS, "/SnowDeformShaders/Private/TS_FFTCopyPixelShader.usf", "MainVertexShader", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FPixelShaderCopyTexturePS, "/SnowDeformShaders/Private/TS_FFTCopyPixelShader.usf", "MainPixelShader", SF_Pixel);

void FCopyTexturePixelShader::DrawToRenderTarget_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef RenderTargetInput, FTextureRHIRef PersistentTargetInput, UTextureRenderTarget2D* RenderTargetPersistent,FVector2D UVOffset)
{
	check(IsInRenderingThread());

	QUICK_SCOPE_CYCLE_COUNTER(STAT_ShaderPlugin_PixelShader); // Used to gather CPU profiling data for the UE4 session frontend
	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_Pixel); // Used to profile GPU activity and add metadata to be consumed by for example RenderDoc

	FShaderResourceViewRHIRef RenderTargetInputRef = RHICreateShaderResourceView(RenderTargetInput, 0);
	FShaderResourceViewRHIRef RenderTargetPersistentRef = RHICreateShaderResourceView(PersistentTargetInput, 0);

	// setup render pass info
	FRHIRenderPassInfo RenderPassInfo(RenderTargetPersistent->GetRenderTargetResource()->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("ShaderPlugin_OutputToRenderTarget"));

	// map our shader to global shader map
	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FSimplePassThroughVS> VertexShader(ShaderMap);
	TShaderMapRef<FPixelShaderCopyTexturePS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the pixel shader
	FPixelShaderCopyTexturePS::FParameters PassParameters;
	PassParameters.RenderTargetInput = RenderTargetInputRef;
	PassParameters.PersistentRTInput = RenderTargetPersistentRef;
	PassParameters.UVOffset = UVOffset;


	UE_LOG(LogTemp, Warning, TEXT("%f, %f"), UVOffset.X, UVOffset.Y);

	SetShaderParameters(RHICmdList, *PixelShader, PixelShader->GetPixelShader(), PassParameters);

	// Draw
	RHICmdList.SetStreamSource(0, GSimpleScreenVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(0, 2, 1);
	// Resolve render target
	RHICmdList.CopyToResolveTarget(RenderTargetPersistent->GetRenderTargetResource()->GetRenderTargetTexture(), RenderTargetPersistent->GetRenderTargetResource()->TextureRHI, FResolveParams());

	RHICmdList.EndRenderPass();
}
