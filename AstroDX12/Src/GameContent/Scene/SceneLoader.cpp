#include "SceneLoader.h"
#include <External/OBJ-Loader/Source/OBJ_Loader.h>

namespace SceneLoaderHelpers
{
	[[nodiscard]] static std::vector<SceneMeshData<VertexData_Pos>> ConvertMeshData_Pos(std::vector<objl::Mesh> meshes)
	{
		std::vector<SceneMeshData<VertexData_Pos>> meshObjects_VD_Pos;
		meshObjects_VD_Pos.reserve(meshes.size());
		for (const auto& mesh : meshes)
		{
			// Vert data
			std::vector<VertexData_Pos> vertsConverted;
			vertsConverted.reserve(mesh.Vertices.size());
			for (const auto& vert : mesh.Vertices)
			{
				// Only read Position
				vertsConverted.push_back(XMFLOAT3(vert.Position.X, vert.Position.Y, vert.Position.Z));
			}

			// Indices
			std::vector<std::uint16_t> indicesConverted;
			indicesConverted.reserve(mesh.Indices.size());
			std::transform(mesh.Indices.begin(), mesh.Indices.end(),
				std::back_inserter(indicesConverted), [](const unsigned int value)
				{
					return static_cast<uint16_t>(value);
				});

			std::string meshName = mesh.MeshName.length() > 0 ? mesh.MeshName : "unnamed Mesh";
			meshObjects_VD_Pos.emplace_back(vertsConverted, indicesConverted, meshName);
		}
		return meshObjects_VD_Pos;
	}

	[[nodiscard]] static std::vector<SceneMeshData<VertexData_Pos_Normal_UV>> ConvertMeshData_PosNormUV(std::vector<objl::Mesh> meshes)
	{
		std::vector<SceneMeshData<VertexData_Pos_Normal_UV>> meshObjects_VD_PosNormUV;
		meshObjects_VD_PosNormUV.reserve(meshes.size());
		for (const auto& mesh : meshes)
		{
			// Vert data
			std::vector<VertexData_Pos_Normal_UV> vertsConverted;
			vertsConverted.reserve(mesh.Vertices.size());
			for (const auto& vert : mesh.Vertices)
			{
				vertsConverted.emplace_back(
					XMFLOAT3(vert.Position.X, vert.Position.Y, vert.Position.Z),
					XMFLOAT3(vert.Normal.X, vert.Normal.Y, vert.Normal.Z),
					XMFLOAT2(vert.TextureCoordinate.X, vert.TextureCoordinate.Y)
				);
			}

			// Indices
			std::vector<std::uint16_t> indicesConverted;
			indicesConverted.reserve(mesh.Indices.size());
			std::transform(mesh.Indices.begin(), mesh.Indices.end(),
				std::back_inserter(indicesConverted), [](const unsigned int value)
				{
					return static_cast<uint16_t>(value);
				});

			std::string meshName = mesh.MeshName.length() > 0 ? mesh.MeshName : "unnamed Mesh";
			meshObjects_VD_PosNormUV.emplace_back(vertsConverted, indicesConverted, meshName);
		}
		return meshObjects_VD_PosNormUV;
	}
}

SceneData SceneLoader::LoadScene(std::uint8_t SceneIdx)
{
	assert((SceneIdx > 0 && SceneIdx < 2) && "Invalid Scene Index, cannot load the scene");
	switch (SceneIdx)
	{
	case 1:
		return SceneLoader::LoadScene1();

	}

	return SceneData();
}

SceneData SceneLoader::LoadScene1()
{
	objl::Loader loader;
	assert(loader.LoadFile("Content/Meshes/teapot.obj") && "Failed to load obj file");
	
	SceneData sd;
	sd.SceneMeshObjects_VD_PosNormUV = SceneLoaderHelpers::ConvertMeshData_PosNormUV(loader.LoadedMeshes);
	return sd;
}
