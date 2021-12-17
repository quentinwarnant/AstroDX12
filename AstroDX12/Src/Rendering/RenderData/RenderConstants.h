#pragma once

#include <Maths/MathUtils.h>

struct RenderPassConstants
{

};

struct RenderableObjectConstantData
{
	DirectX::XMFLOAT4X4 WorldViewProj = AstroTools::Maths::Identity4x4();
};
