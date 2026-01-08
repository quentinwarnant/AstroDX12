#pragma once

#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/RenderData/GeometryHelper.h>
#include <Rendering/Common//RendererContext.h>

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

	class CommonMeshes
	{
	public:
		static void CreateCommonMeshes(MeshLibrary& meshLibrary, RendererContext& rendererContext )
		{
			auto sphereGeo = CreateSphere();
			meshLibrary.AddMesh(rendererContext, CommonMeshNamesMap[Sphere], sphereGeo.vertices, sphereGeo.indices);

		}

	private:
		static Geometry CreateSphere()
		{
			return GeometryHelper::GenerateDelaunaySphere(4);
		}

	};

}