#pragma once

#include <Maths/MathUtils.h>

using namespace DirectX;

struct RenderPassConstants
{
	XMFLOAT4X4 View = AstroTools::Maths::Identity4x4();
	XMFLOAT4X4 Proj = AstroTools::Maths::Identity4x4();
	XMFLOAT4X4 InvView = AstroTools::Maths::Identity4x4();
	XMFLOAT4X4 InvProj = AstroTools::Maths::Identity4x4();
	XMFLOAT4X4 ViewProj = AstroTools::Maths::Identity4x4();
	XMFLOAT4X4 InvViewProj = AstroTools::Maths::Identity4x4();

	XMFLOAT3 EyePosWorld = {0.0f, 0.0f, 0.0f};
	float padding1 = 0.0f;
	XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
};

struct RenderableObjectConstantData
{
	XMFLOAT4X4 WorldTransform = AstroTools::Maths::Identity4x4();
};
