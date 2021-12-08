#pragma once

#include <Common.h>

using namespace DirectX;

struct VertexData_Short_POD
{
	VertexData_Short_POD()
		: Position(0,0,0)
		, Color(0,0,0,0)
	{}

	VertexData_Short_POD( XMFLOAT3& Pos,  XMFLOAT4& Col)
		: Position(Pos)
		, Color(Col)
	{}

	XMFLOAT3 Position;
	XMFLOAT4 Color;
};


struct VertexData_Long_POD
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
	//More stuff like tangent, normal, uv, ...
};

class IVertexData
{
public:
	virtual ~IVertexData() = default;
	virtual const std::shared_ptr<void>& GetData() = 0;
};

class VertexData_Short : public IVertexData
{
public:
	VertexData_Short(XMFLOAT3 pos, XMFLOAT4 col)
		: POD{ std::make_shared<VertexData_Short_POD>(pos, col) }
	{
	}

	virtual ~VertexData_Short() = default;
	
	virtual const std::shared_ptr<void>& GetData() override
	{
		return POD; 
	}

private:
	std::shared_ptr<VertexData_Short_POD> POD;
	//VertexData_Short_POD POD;
};

