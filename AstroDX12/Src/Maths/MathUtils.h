#pragma once

#include "../Common.h"

using namespace DirectX;

namespace AstroTools
{
	namespace Maths
	{
		constexpr float Pi = XM_PI;

		static XMFLOAT4X4 Identity4x4()
		{
			constexpr XMFLOAT4X4 Identity = XMFLOAT4X4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);
			return Identity;
		}

		static float Clamp(float x, float min, float max)
		{
			return x < min ? min : (x > max ? max : x);
		}
	};
}