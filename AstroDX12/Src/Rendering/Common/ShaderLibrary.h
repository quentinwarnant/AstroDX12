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
		ComPtr<IDxcBlob> GetCompiledShader(
			const std::wstring& path,
			const std::wstring& entryPoint,
			const std::vector<std::wstring>& defines,
			const std::wstring& target )
		{
			auto key = path + target + entryPoint;
			for(auto& define : defines)
			{
				key += define;
			}
			if ( const auto& shaderIt = m_compiledShaders.find(key); shaderIt != m_compiledShaders.end())
			{
				return (*shaderIt).second;
			}

			auto compiledShader = AstroTools::Rendering::CompileShader(path, defines, entryPoint, target);
			m_compiledShaders.emplace(key, compiledShader);
			return compiledShader;
		}

	private:
		std::map<std::wstring, ComPtr<IDxcBlob>> m_compiledShaders;
	};
}
