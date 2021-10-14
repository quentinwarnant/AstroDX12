#pragma once
#include "../../Common.h"

using namespace DirectX;

struct IVertexDataInterface
{

};

struct VertexData_Short : public IVertexDataInterface
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};