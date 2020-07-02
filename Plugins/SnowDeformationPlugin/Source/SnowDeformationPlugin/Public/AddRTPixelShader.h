#pragma once
#include <Runtime\Engine\Classes\Engine\TextureRenderTarget2D.h>

/**************************************************************************************/
/* This is just an interface we use to keep all the pixel shading code in one file.   */
/**************************************************************************************/

class FCopyTexturePixelShader
{
public:

    static void DrawToRenderTarget_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIRef RenderTargetInput, FTextureRHIRef PersistentTargetInput, UTextureRenderTarget2D* RenderTargetPersistent,FVector2D UVOffset);

};