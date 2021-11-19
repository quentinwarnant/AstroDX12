#pragma once
#include "../../Common.h"

using namespace DirectX;

struct IVertexDataInterface
{
	virtual ULONG64 GetDataSize() const = 0;
	virtual ~IVertexDataInterface() = default;
};

struct VertexData_Short : public IVertexDataInterface
{
	VertexData_Short(XMFLOAT3 pos, XMFLOAT4 col) 
		: Position(pos)
		, Color(col)
	{
	}

	virtual ~VertexData_Short() = default;

	XMFLOAT3 Position;
	XMFLOAT4 Color;

	virtual ULONG64 GetDataSize() const override
	{
		return sizeof(XMFLOAT3) + sizeof(XMFLOAT4);
	}

};