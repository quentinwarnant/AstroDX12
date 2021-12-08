#pragma once

#include <Common.h>
#include <Rendering/Common/UploadBuffer.h>

using Microsoft::WRL::ComPtr;

class IRenderable;
class IVertexData;
class Mesh;

class IRenderer
{
public:
	virtual ~IRenderer() {};
	virtual void Init(HWND window, int width, int height) = 0;
	virtual void FinaliseInit() = 0;
	virtual void Render(
		float deltaTime,
		std::vector<std::shared_ptr<IRenderable>>& renderableObjects) = 0;
	virtual void FlushRenderQueue() = 0;
	virtual void Shutdown() = 0;
	virtual void CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature) = 0;
	virtual void CreateMesh(std::unique_ptr<Mesh>& mesh, const void* vertexData, const UINT vertexDataCount, const UINT vertexDataByteSize, const std::vector<std::uint16_t>& indices) = 0;
protected:
	virtual ComPtr<ID3D12Device> GetDevice() const = 0;

public:
	template<typename T>
	std::unique_ptr<UploadBuffer<T>> CreateConstantBuffer(UINT elementCount)
	{
		return std::make_unique<UploadBuffer<T>>(GetDevice().Get(), elementCount, true);
	}

	virtual void CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc) = 0;
	virtual void CreateGraphicsPipelineState(
		ComPtr<ID3D12PipelineState>& pso,
		ComPtr<ID3D12RootSignature>& rootSignature,
		std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
		ComPtr<ID3DBlob>& vertexShaderByteCode,
		ComPtr<ID3DBlob>& pixelShaderByteCode) = 0;
};
