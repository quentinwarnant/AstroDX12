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
			const std::string& filename,
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
			auto filenameWStr = s2ws(filename);
			hr = D3DCompileFromFile(filenameWStr.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
				entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

			if (errors != nullptr)
				OutputDebugStringA((char*)errors->GetBufferPointer());

			DX::ThrowIfFailed(hr);

			return byteCode;
		}

		static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			const void* initData,
			UINT64 byteSize,
			Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer
		)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

			// Create default buffer resource
			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto defaultHeapBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&defaultHeapBufferDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

			// In order to copy CPU memory data into our default buffer, we need to create
			// an intermediate upload heap. 
			auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto uploadHeapBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&uploadHeapBufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

			// Describe the data we want to copy into the default buffer.
			D3D12_SUBRESOURCE_DATA subResourceData = {};
			subResourceData.pData = initData;
			subResourceData.RowPitch = byteSize;
			subResourceData.SlicePitch = subResourceData.RowPitch;

			// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
			// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
			// the intermediate upload heap data will be copied to mBuffer.
			auto barrierTransitionCommonToCopy = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			commandList->ResourceBarrier(1, &barrierTransitionCommonToCopy);
			UpdateSubresources<1>(commandList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
			auto barrierTransitionCopyToRead = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
			commandList->ResourceBarrier(1, &barrierTransitionCopyToRead);

			// Note: uploadBuffer has to be kept alive after the above function calls because
			// the command list has not been executed yet that performs the actual copy.
			// The caller can Release the uploadBuffer after it knows the copy has been executed.


			return defaultBuffer;
		}
	};
}