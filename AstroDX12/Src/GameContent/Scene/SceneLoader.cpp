#include "SceneLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <fstream>
#include <ios>
#include <iostream>

#include <GameContent\Scene\SceneDescription.h>

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

	[[nodiscard]] static XMFLOAT3 ConvertStringToFloat3(const std::string& str)
	{
		const size_t yIndex = str.find(',', 0) + 1;
		const size_t zIndex = str.find(',', yIndex) + 1;

		XMFLOAT3 result;
		result.x = std::stof(str.substr(0, yIndex - 1));
		result.y = std::stof(str.substr(yIndex, zIndex - yIndex - 1));
		result.z = std::stof(str.substr(zIndex, str.length() - zIndex));

		return result;
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
	SceneDescription sceneDesc;
	{
		std::ifstream stream;
		stream.open(DX::GetWorkingDirectory()+"/Content/Scenes/Scene1.lvl", std::ifstream::in);

		SceneObjectDesc newObject;
		while (stream.good())
		{
			char line[64];
			stream.getline(line,64);

			std::string text{ line };

			if (text == "#")
			{
				newObject = {};
			}
			else if (text == "/")
			{
				// finalise object and add to the list
				sceneDesc.sceneObjects.push_back(newObject);
			}
			else
			{
				// parse lines and initialise parameters of newObject
				stream.getline(line, 64);
				std::string propertyVal{ line };

				if (text == "MeshPath")
				{
					newObject.meshPath = propertyVal;
				}
				else if (text == "Position")
				{
					newObject.position = SceneLoaderHelpers::ConvertStringToFloat3(propertyVal);
				}
				else if (text == "Rotation")
				{
					XMFLOAT3 eulerAnglesInDegrees = SceneLoaderHelpers::ConvertStringToFloat3(propertyVal);
					newObject.rotationEulerAngles = { XMConvertToRadians(eulerAnglesInDegrees.x),XMConvertToRadians(eulerAnglesInDegrees.y),XMConvertToRadians(eulerAnglesInDegrees.z) };
				}
				else if (text == "Scale")
				{
					newObject.scale = SceneLoaderHelpers::ConvertStringToFloat3(propertyVal);
				}
			}
		}

		stream.close();
	}


	SceneData sd;
	Assimp::Importer importer;
	std::vector<aiMesh*> sceneMeshes;
	for (int16_t idx = 0; idx < sceneDesc.sceneObjects.size(); ++idx)
	{
		const aiScene* meshScene = importer.ReadFile(sceneDesc.sceneObjects[idx].meshPath, 0);
		assert(meshScene && importer.GetErrorString());

		const XMFLOAT3 pos = sceneDesc.sceneObjects[idx].position;
		const XMFLOAT3 rot = sceneDesc.sceneObjects[idx].rotationEulerAngles;
		const XMFLOAT3 scale = sceneDesc.sceneObjects[idx].scale;

		const XMVECTOR vTranslation = XMLoadFloat3(&pos);
		const XMVECTOR vRotation = XMLoadFloat3(&rot); // Euler angles (radians)
		const XMVECTOR vScale = XMLoadFloat3(&scale);
		const XMMATRIX matTranslation = XMMatrixTranslationFromVector(vTranslation);
		const XMMATRIX matRotation = XMMatrixRotationRollPitchYawFromVector(vRotation);
		const XMMATRIX matScale = XMMatrixScalingFromVector(vScale);

		// HLSL uses Column major matrices, XMMaths uses Row major matrices - so transposing it to be compatible
		const XMMATRIX transformMat = XMMatrixTranspose(matScale * matRotation * matTranslation);
		XMFLOAT4X4 transform;
		XMStoreFloat4x4(&transform, transformMat);

		for (int64_t meshIdx = 0; meshIdx < meshScene->mNumMeshes; ++meshIdx)
		{
			auto convertedMesh = SceneLoaderHelpers::ConvertMeshData_PosNormUV(meshScene->mMeshes[meshIdx]);
			convertedMesh.transform = transform;
			sd.SceneMeshObjects_VD_PosNormUV.push_back(convertedMesh);
		}
	}

	return sd;
}
