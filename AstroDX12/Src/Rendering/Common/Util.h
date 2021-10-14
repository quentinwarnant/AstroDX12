#pragma once

#include "../../Common.h"

namespace AstroTools
{
	class Util
	{
		static UINT CalcConstantBufferByteSize(UINT dataSize)
		{
			//round up to nearest
			// multiple of 256.  We do this by adding 255 and then masking off
			// the lower 2 bytes which store all bits < 256.
			return (dataSize + 255) & ~255;
		}
	};
}