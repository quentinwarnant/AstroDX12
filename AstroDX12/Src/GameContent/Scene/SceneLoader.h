#pragma once

#include <Common.h>
#include "Rendering\RenderData\VertexData.h"

template<class VertexData_Type>
struct SceneMeshData
{
	SceneMeshData() = default;

	SceneMeshData(
		std::vector<VertexData_Type> inVerts,
		std::vector<std::uint16_t> inIndices,
		std::string inMeshName)
		: verts(std::move(inVerts))
		, indices(std::move(inIndices))
		, transform()
		, meshName(std::move(inMeshName))
	{}

	virtual ~SceneMeshData() = default;

	std::vector<VertexData_Type> verts;
	std::vector<std::uint16_t> indices;
	XMFLOAT4X4 transform;
	std::string meshName;
	
};

struct SceneData
{
	std::vector<SceneMeshData<VertexData_Pos>> SceneMeshObjects_VD_Pos;
	std::vector<SceneMeshData<VertexData_Short>> SceneMeshObjects_VD_Short;
	std::vector<SceneMeshData<VertexData_Pos_Normal_UV>> SceneMeshObjects_VD_PosNormUV;
	//...
};

class SceneLoader
{
public:
	[[nodiscard]] static SceneData LoadScene(std::uint8_t SceneIdx);

private:
	[[nodiscard]] static SceneData LoadScene1();
};

