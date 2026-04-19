#pragma once

namespace AstroDX
{
	enum CommonMeshNames
	{
		Sphere,
		Cube
	};

	static std::map<CommonMeshNames, std::string> CommonMeshNamesMap
	{
		{CommonMeshNames::Sphere, "Sphere"},
		{CommonMeshNames::Cube, "Cube"},
	};
}