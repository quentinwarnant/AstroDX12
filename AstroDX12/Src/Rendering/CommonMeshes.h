#pragma once

#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/CommonMeshNames.h>
#include <Rendering/Common//RendererContext.h>
#include <Rendering/RenderData/GeometryHelper.h>

namespace AstroDX
{
	class CommonMeshes
	{
	public:
		static void CreateCommonMeshes(MeshLibrary& meshLibrary, RendererContext& rendererContext )
		{
			auto sphereGeo = GeometryHelper::GenerateDelaunaySphere(4);
			meshLibrary.AddMesh(rendererContext, CommonMeshNamesMap[Sphere], sphereGeo.vertices, sphereGeo.indices);

			auto cubeGeo = GeometryHelper::GenerateCube();
			meshLibrary.AddMesh(rendererContext, CommonMeshNamesMap[Cube], cubeGeo.vertices, cubeGeo.indices);
		}

		static bool GetCommonMesh( const MeshLibrary& meshLibrary, const CommonMeshNames meshName, std::weak_ptr<IMesh>& OutMesh)
		{
			return meshLibrary.GetMesh(CommonMeshNamesMap[meshName], OutMesh);
		}

	};

}