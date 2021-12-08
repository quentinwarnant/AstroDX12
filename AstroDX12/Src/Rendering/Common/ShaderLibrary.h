#pragma once

#include <map>
#include <string>

#include <Common.h>
#include <Rendering/Common/RenderingUtils.h>

using namespace Microsoft::WRL;
namespace AstroTools::Rendering
{
	class ShaderLibrary
	{
	public:
		ComPtr<ID3DBlob> GetCompiledShader(
			const std::string& path,
			const std::string& entryPoint,
			const std::string& target )
		{
			const auto key = path + target;
			if ( const auto& shaderIt = m_compiledShaders.find(key); shaderIt != m_compiledShaders.end())
			{
				return (*shaderIt).second;
			}

			auto compiledShader = AstroTools::Rendering::CompileShader(path, nullptr, entryPoint, target);
			m_compiledShaders.emplace(key, compiledShader);
			return compiledShader;
		}

	private:
		std::map<std::string, ComPtr<ID3DBlob>> m_compiledShaders;
	};
}
