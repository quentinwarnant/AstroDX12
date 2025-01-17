#include "SceneLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace SceneLoaderHelpers
{
	[[nodiscard]] static std::vector<uint32_t> GetMeshIndices(aiMesh* mesh)
	{
		assert(mesh->HasFaces() && "Mesh has no face data, cannot access vert indices");
		std::vector<uint32_t> indicesConverted;
		indicesConverted.reserve(mesh->mNumFaces * 3); // Only supporting triangle faces atm
		for (uint64_t faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx)
		{
			assert(mesh->mFaces[faceIdx].mNumIndices == 3 && "Mesh face has a non 3 index count face, unsupported!");
			indicesConverted.push_back(static_cast<uint32_t>(mesh->mFaces[faceIdx].mIndices[0]));
			indicesConverted.push_back(static_cast<uint32_t>(mesh->mFaces[faceIdx].mIndices[1]));
			indicesConverted.push_back(static_cast<uint32_t>(mesh->mFaces[faceIdx].mIndices[2]));
		}
		return indicesConverted;
	}

	[[nodiscard]] static std::string GetMeshName(aiMesh* mesh)
	{
		return mesh->mName.length > 0 ? mesh->mName.C_Str() : "unnamed Mesh";
	}

	[[nodiscard]] static SceneMeshData<VertexData_Pos> ConvertMeshData_Pos(aiMesh* mesh)
	{
		// Vert data
		std::vector<VertexData_Pos> vertsConverted;
		assert(mesh->mNumVertices > 0 && "No vertices in mesh");
		vertsConverted.reserve(mesh->mNumVertices);
		for (uint64_t vertIdx = 0; vertIdx < mesh->mNumVertices; ++vertIdx)
		{
			// Only read Position
			const auto vert = mesh->mVertices[vertIdx];
			vertsConverted.push_back(XMFLOAT3(vert.x, vert.y, vert.z));
		}

		// Indices
		std::vector<uint32_t> indicesConverted = SceneLoaderHelpers::GetMeshIndices(mesh);

		// Name 
		std::string meshName = SceneLoaderHelpers::GetMeshName(mesh);
		SceneMeshData<VertexData_Pos> meshObjects_VD_Pos = { vertsConverted, indicesConverted, meshName };
		return meshObjects_VD_Pos;
	}

	[[nodiscard]] static SceneMeshData<VertexData_Pos_Normal_UV> ConvertMeshData_PosNormUV(aiMesh* mesh)
	{
			// Vert data
			assert(mesh->mNumVertices > 0 && "No vertices in mesh");
			std::vector<VertexData_Pos_Normal_UV> vertsConverted;
			vertsConverted.reserve(mesh->mNumVertices);
			for (uint64_t vertIdx = 0; vertIdx < mesh->mNumVertices; ++vertIdx)
			{
				const auto vert = mesh->mVertices[vertIdx];
				assert(mesh->HasNormals() && "Mesh has no normals but we expect to load some");
				const auto normal = mesh->mNormals[vertIdx];
				constexpr uint16_t UVIndex = 0;
				const bool hasUVs = mesh->HasTextureCoords(UVIndex);
				const auto uvs = hasUVs ? mesh->mTextureCoords[UVIndex][vertIdx] : aiVector3D(0,0,0);

				vertsConverted.emplace_back(
					XMFLOAT3(vert.x, vert.y, vert.z),
					XMFLOAT3(normal.x, normal.y, normal.z),
					XMFLOAT2(uvs.x, uvs.y)
				);
			}

			// Indices
			std::vector<uint32_t> indicesConverted = SceneLoaderHelpers::GetMeshIndices(mesh);
			
			// Name
			std::string meshName = SceneLoaderHelpers::GetMeshName(mesh);

			SceneMeshData<VertexData_Pos_Normal_UV> meshObject_VD_PosNormUV = { vertsConverted, indicesConverted, meshName};
			return meshObject_VD_PosNormUV;
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
	const std::vector< std::string > meshPathsToLoad =
	{
		{"Content/Meshes/spider.fbx"},
//		{"Content/Meshes/box.fbx"}
	};

	float rotAngle = 15.f;
	float c = cosf(rotAngle);
	float s = sinf(rotAngle);

	const std::vector<XMFLOAT3> translation =
	{
		{0.f,0.f,0.f},
		{10.f,0.f,0.f}
	};

	const std::vector<float> rotationXAxis =
	{
		{0.f},
		{30.f}
	};

	const std::vector<XMFLOAT3> scale =
	{
		{1.f,0.f,0.f},
		{0.02f,0.f,0.f}
	};


	SceneData sd;
	Assimp::Importer importer;
	std::vector<aiMesh*> sceneMeshes;
	for (int16_t idx = 0; idx <meshPathsToLoad.size(); ++idx)
	{
		const aiScene* meshScene = importer.ReadFile(meshPathsToLoad[idx], 0);
		assert(meshScene && importer.GetErrorString() );

		XMFLOAT4X4 transform = XMFLOAT4X4(
			scale[idx].x, 0.0f, 0.0f, translation[idx].x,
			0.0f, scale[idx].y + cosf(rotationXAxis[idx]), -sinf(rotationXAxis[idx]), translation[idx].y,
			0.0f, sinf(rotationXAxis[idx]), scale[idx].z + cosf(rotationXAxis[idx]), translation[idx].z,
			0.0f, 0.0f, 0.0f, 1.f);


		for (int64_t meshIdx = 0; meshIdx < meshScene->mNumMeshes; ++meshIdx)
		{
			auto convertedMesh = SceneLoaderHelpers::ConvertMeshData_PosNormUV(meshScene->mMeshes[meshIdx]);
			convertedMesh.transform = transform;
			sd.SceneMeshObjects_VD_PosNormUV.push_back(convertedMesh);
		}
	}

	return sd;
}
