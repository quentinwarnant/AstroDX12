#pragma once
#include "../../Common.h"

using namespace DirectX;

struct VertexData_Short
{
	VertexData_Short(XMFLOAT3 pos, XMFLOAT4 col) 
		: Position(pos)
		, Color(col)
	{
	}

	XMFLOAT3 Position;
	XMFLOAT4 Color;
};