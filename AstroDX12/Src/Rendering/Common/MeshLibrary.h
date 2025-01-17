#pragma once
#include <map>
#include <memory>
#include <string>
#include <cassert>

#include <Rendering/Common/RendererContext.h>
#include <Rendering/RenderData/Mesh.h>

class MeshLibrary
{
public:
	template <typename MeshVertexDataType>
	std::weak_ptr<IMesh> AddMesh(
		RendererContext& rendererContext,
		const std::string& meshName,
		const std::vector<MeshVertexDataType>& vertexMeshData,
		const std::vector<uint32_t> vertexIndices )
	{
		const auto it = Meshes.find(meshName);
		assert(it == Meshes.end()); // Duplicate not allowed
		auto entryIt = Meshes.emplace(meshName, std::make_shared<Mesh<MeshVertexDataType>>(
			rendererContext,
			meshName,
			vertexMeshData,// vertexMeshData.size(), sizeof(vertexMeshData[0]),
			vertexIndices
		)).first;

		return std::weak_ptr<IMesh>(entryIt->second);
	}



	bool GetMesh(const std::string& meshName, std::weak_ptr<IMesh>& OutMesh) const
	{
		auto it = Meshes.find(meshName);
		if (it == Meshes.end())
		{
			return false;
		}
		OutMesh = std::weak_ptr<IMesh>((*it).second);
		return true;
	}
private:
	std::map<std::string,std::shared_ptr<IMesh>> Meshes;
};