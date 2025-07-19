#pragma once

#include <Common.h>

#include <DXC/d3d12shader.h>
#include <DXC/dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

#include <string> 
#include <fstream>

using namespace Microsoft::WRL;

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

		static ComPtr<IDxcBlob> CompileShader(
			const std::wstring& filename,
			const std::vector<std::wstring>& defines,
			const std::wstring& entrypoint,
			const std::wstring& targetProfile)
		{

			ComPtr<IDxcUtils> utils;
			{
				const HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.GetAddressOf()));
				DX::ThrowIfFailed(hr);
			}
			ComPtr<IDxcCompiler3> compiler;
			{
				const HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(compiler.GetAddressOf()));
				DX::ThrowIfFailed(hr);
			}


			// Load file
			ComPtr<IDxcBlobEncoding> sourceBlob;
			{
				const HRESULT hr = utils->LoadFile(filename.c_str(), nullptr, &sourceBlob );
				DX::ThrowIfFailed(hr);
			}

			DxcBuffer sourceBuffer
			{
				.Ptr = sourceBlob->GetBufferPointer(),
				.Size = sourceBlob->GetBufferSize(),
				.Encoding = 0u,
			};

			// Compile Shader 
			std::vector<LPCWSTR> compilationArguments
			{
				L"-E",
				entrypoint.c_str(),
				L"-T",
				targetProfile.c_str(),
				DXC_ARG_PACK_MATRIX_COLUMN_MAJOR,
				DXC_ARG_WARNINGS_ARE_ERRORS,
				DXC_ARG_ALL_RESOURCES_BOUND,
			};

			for (const std::wstring& define : defines)
			{
				compilationArguments.push_back(L"-D");
				compilationArguments.push_back(define.c_str());
			}

#if defined(DEBUG) || defined(_DEBUG)  
				compilationArguments.push_back(DXC_ARG_DEBUG);
#else
				compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
#endif
			
			ComPtr<IDxcResult> compiledShaderBuffer{};
			{
				const HRESULT hr = compiler->Compile(&sourceBuffer,
					compilationArguments.data(),
					static_cast<uint32_t>(compilationArguments.size()),
					nullptr,
					IID_PPV_ARGS(&compiledShaderBuffer));
				DX::ThrowIfFailed(hr);

				if (FAILED(hr))
				{
					DX::astro_assert(false, (std::wstring(L"Failed to compile shader with path : ") + filename.data() ).c_str() );
				}
			}

			// Get compilation errors (if any).
			{
				ComPtr<IDxcBlobUtf8> errors{};
				const HRESULT hr = compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
				DX::ThrowIfFailed(hr);
				if (errors && errors->GetStringLength() > 0)
				{
					const LPCSTR errorMessage = errors->GetStringPointer();
					OutputDebugStringA(errorMessage);
					DX::astro_assert(false, errorMessage);

				}
			}

			// At this point we could extract reflection data from the compiled shader buffer object to get the input name, constant buffer names, parameters, etc
			// TODO: doing this let's us automate the root signature description - which removes fiddly, manual configurations.
			// see:https://rtarun9.github.io/blogs/shader_reflection/

			
			ComPtr<IDxcBlob> compiledShaderBlob{ nullptr };
			{
				const HRESULT hr = compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);
				DX::ThrowIfFailed(hr);
			}

			// Save out the symbols
			{
				ComPtr<IDxcBlob> symbolsData;
				ComPtr<IDxcBlobUtf16> symbolsDataPath;
				const HRESULT hr = compiledShaderBuffer->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(symbolsData.GetAddressOf()), symbolsDataPath.GetAddressOf());
				DX::ThrowIfFailed(hr);

				const LPCWSTR symbolNamePtr = symbolsDataPath.Get()->GetStringPointer();
				auto symbolNameAsWString = std::wstring(symbolNamePtr);

				auto filePath = s2ws(DX::GetWorkingDirectory());
				filePath.append(L"/ShaderSymbols/");
				filePath.append(symbolNameAsWString);

				std::ofstream file(filePath, std::ios::binary | std::ios::out);

				assert(file.is_open());

				file.write((const char*)symbolsData->GetBufferPointer(), symbolsData->GetBufferSize());
				file.close();
			}

			return compiledShaderBlob;
		}

		static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
			ID3D12Device* device,
			ID3D12GraphicsCommandList* commandList,
			const void* initData,
			UINT64 byteSize,
			bool enableUAVsupport,
			Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer
		)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

			// Create default buffer resource
			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto defaultHeapBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
			if (enableUAVsupport)
			{
				defaultHeapBufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			}
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


		[[nodiscard]] static Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTarget(
			ID3D12Device* device,
			UINT64 width,
			UINT64 height,
			DXGI_FORMAT format,
			bool initialStateIsUAV = true
		)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> renderTargetResource;

			// Create default buffer resource
			auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, (UINT)height);
			bufferDesc.Flags = (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				initialStateIsUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_RENDER_TARGET,
				nullptr,
				IID_PPV_ARGS(renderTargetResource.GetAddressOf())));

			return renderTargetResource;
		}

	};
}