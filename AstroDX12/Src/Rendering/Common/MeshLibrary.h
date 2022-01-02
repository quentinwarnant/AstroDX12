#pragma once
#include <map>
#include <memory>
#include <string>
#include <cassert>

#include <Rendering/RenderData/Mesh.h>

class MeshLibrary
{
public:
	std::weak_ptr<Mesh> AddMesh(const std::string& meshName)
	{
		const auto it = Meshes.find(meshName);
		assert(it == Meshes.end()); // Duplicate
		auto entryIt = Meshes.emplace(meshName, std::make_shared<Mesh>()).first;
		entryIt->second->Name = meshName;

		return std::weak_ptr<Mesh>(entryIt->second);
	}

	void AddMesh(const std::string& meshName, std::shared_ptr<Mesh> meshPtr)
	{
		const auto it = Meshes.find(meshName);
		assert(it == Meshes.end()); // Duplicate
		meshPtr->Name = meshName;

		Meshes.emplace(meshName, std::move(meshPtr));
	}

	std::weak_ptr<Mesh> GetMesh(const std::string& meshName) const
	{
		auto it = Meshes.find(meshName);
		assert(it != Meshes.end()); // mesh not found

		return std::weak_ptr<Mesh>((*it).second);
	}
private:
	std::map<std::string,std::shared_ptr<Mesh>> Meshes;
};