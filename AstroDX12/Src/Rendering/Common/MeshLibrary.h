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

	bool GetMesh(const std::string& meshName, std::weak_ptr<Mesh>& OutMesh) const
	{
		auto it = Meshes.find(meshName);
		if (it == Meshes.end())
		{
			return false;
		}
		OutMesh = std::weak_ptr<Mesh>((*it).second);
		return true;
	}
private:
	std::map<std::string,std::shared_ptr<Mesh>> Meshes;
};