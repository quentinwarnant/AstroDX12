#pragma once

#include "../../Common.h"

#include <D3DCompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

namespace AstroTools
{
	namespace Rendering
	{
		static UINT CalcConstantBufferByteSize(UINT dataSize)
		{
			//round up to nearest
			// multiple of 256.  We do this by adding 255 and then masking off
			// the lower 2 bytes which store all bits < 256.
			return (dataSize + 255) & ~255;
		}

		static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
			const std::wstring& filename,
			const D3D_SHADER_MACRO* defines,
			const std::string& entrypoint,
			const std::string& target)
		{
			UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
			compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
			HRESULT hr = S_OK;

			Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
			Microsoft::WRL::ComPtr<ID3DBlob> errors;
			hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
				entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

			if (errors != nullptr)
				OutputDebugStringA((char*)errors->GetBufferPointer());

			DX::ThrowIfFailed(hr);

			return byteCode;
		}
	};
}