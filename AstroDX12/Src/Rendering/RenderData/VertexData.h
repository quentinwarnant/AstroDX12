#pragma once

#include <Common.h>

using namespace DirectX;

struct VertexData_Position_POD
{
	VertexData_Position_POD()
		: Position(0, 0, 0)
	{}

	VertexData_Position_POD(const XMFLOAT3& Pos)
		: Position(Pos)
	{}

	XMFLOAT3 Position;
};

struct VertexData_Short_POD
{
	VertexData_Short_POD()
		: Position(0,0,0)
		, Color(0,0,0,0)
	{}

	VertexData_Short_POD( const XMFLOAT3& Pos,  const XMFLOAT4& Col)
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
	virtual std::shared_ptr<void> GetData() = 0;
};


class VertexData_Pos : public IVertexData
{
public:
	VertexData_Pos()
		: POD{ std::make_shared<VertexData_Position_POD>(XMFLOAT3(0.f, 0.f, 0.f)) }
	{
	}

	VertexData_Pos(XMFLOAT3 pos)
		: POD{ std::make_shared<VertexData_Position_POD>(pos) }
	{
	}

	virtual ~VertexData_Pos() = default;

	virtual std::shared_ptr<void> GetData() override
	{
		return POD;
	}

private:
	std::shared_ptr<VertexData_Position_POD> POD;
	
};

class VertexData_Short : public IVertexData
{
public:
	VertexData_Short(XMFLOAT3 pos, XMFLOAT4 col)
		: POD{ std::make_shared<VertexData_Short_POD>(pos, col) }
	{
	}

	virtual ~VertexData_Short() = default;
	
	virtual std::shared_ptr<void> GetData() override
	{
		return POD; 
	}

private:
	std::shared_ptr<VertexData_Short_POD> POD;
};

