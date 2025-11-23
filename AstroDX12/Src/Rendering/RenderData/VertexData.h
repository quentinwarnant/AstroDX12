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


struct VertexData_Position_UV_POD
{
	VertexData_Position_UV_POD()
		: Position(0, 0, 0)
		, UV(0, 0)
	{
	}

	VertexData_Position_UV_POD(const XMFLOAT3& inPos, const XMFLOAT2& inUV)
		: Position(inPos)
		, UV(inUV)
	{
	}

	XMFLOAT3 Position;
	XMFLOAT2 UV;
};

struct VertexData_Position_Normal_UV_POD
{
	VertexData_Position_Normal_UV_POD()
		: Position(0, 0, 0)
		, Normal(0, 0, 0)
		, UV( 0, 0)
	{}

	VertexData_Position_Normal_UV_POD(const XMFLOAT3& inPos, const XMFLOAT3& inNormal, const XMFLOAT2& inUV)
		: Position(inPos)
		, Normal(inNormal)
		, UV(inUV)
	{}

	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 UV;

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

class VertexData_Pos_Normal_UV : public IVertexData
{
public:
	/*VertexData_Pos_Normal_UV_Pos()
		: POD{ std::make_shared<VertexData_Position_Normal_UV_POD>() }
	{
	}*/

	VertexData_Pos_Normal_UV(XMFLOAT3 pos, XMFLOAT3 normal, XMFLOAT2 uv)
		: POD{ std::make_shared<VertexData_Position_Normal_UV_POD>(pos, normal, uv) }
	{
	}

	virtual ~VertexData_Pos_Normal_UV() = default;

	virtual std::shared_ptr<void> GetData() override
	{
		return POD;
	}

private:
	std::shared_ptr<VertexData_Position_Normal_UV_POD> POD;

};