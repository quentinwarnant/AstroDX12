#pragma once

#include <string>
#include <vector>
#include <Common.h>

class SceneObjectDesc
{
public:
	std::string meshPath;
	XMFLOAT3 position;
	XMFLOAT3 rotationEulerAngles;
	XMFLOAT3 scale;
};

class SceneDescription
{
public:
	std::vector<SceneObjectDesc> sceneObjects;
};

