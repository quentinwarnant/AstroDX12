#pragma once

#include <Common.h>
#include <Rendering/Common/UploadBuffer.h>
#include <Rendering/Common/RenderTarget.h>
#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/RendererProcessedObjectType.h>
#include <functional>
#include <map>
using Microsoft::WRL::ComPtr;

struct FrameResource;
class IStructuredBuffer;
struct RendererContext;
class GPUPass;

class IRenderer
{
public:
	virtual ~IRenderer() {};
	virtual void Init(HWND window, int width, int height) = 0;
	virtual void FinaliseInit() = 0;
	virtual void StartNewFrame(FrameResource* frameResources) = 0;
	virtual void EndNewFrame(std::function<void(int)> onNewFenceValue) = 0;
	
	virtual void ProcessGPUPass(
		const GPUPass& pass,
		const FrameResource& frameResources) = 0;
	
	virtual void AddNewFence(std::function<void(int)> onNewFenceValue) = 0;
	virtual void Shutdown() = 0;
	virtual void CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature) = 0;
	
	virtual RendererContext GetRendererContext() = 0;
protected:
	virtual ComPtr<ID3D12Device> GetDevice() const = 0;
	virtual void CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc) = 0;

public:
	virtual void Create_const_uav_srv_BufferDescriptorHeaps() = 0;

	template<typename T>
	std::unique_ptr<UploadBuffer<T>> CreateConstantBuffer(UINT elementCount)
	{
		return std::make_unique<UploadBuffer<T>>(GetDevice().Get(), elementCount, true);
	}

	virtual D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress, UINT cbvByteSize) = 0;
	virtual void CreateStructuredBufferAndViews(IStructuredBuffer* structuredBuffer, bool srv, bool uav) = 0;
	virtual void CreateGraphicsPipelineState(
		ComPtr<ID3D12PipelineState>& pso,
		ComPtr<ID3D12RootSignature>& rootSignature,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>* inputLayout,
		ComPtr<IDxcBlob>& vertexShaderByteCode,
		ComPtr<IDxcBlob>& pixelShaderByteCode) = 0;
	virtual void CreateComputePipelineState(
		ComPtr<ID3D12PipelineState>& pso,
		ComPtr<ID3D12RootSignature>& rootSignature,
		ComPtr<IDxcBlob>& computeShaderByteCode) = 0;
	virtual void BuildFrameResources(std::vector<std::unique_ptr<FrameResource>>& outFrameResourcesList, int frameResourcesCount, int renderableObjectCount, int computableObjectCount) = 0;
	virtual void InitialiseRenderTarget(
		std::weak_ptr<RenderTarget> renderTarget,
		LPCWSTR name,
		UINT32 width,
		UINT32 height,
		DXGI_FORMAT format,
		bool initialStateIsUAV = true) = 0; 
	virtual UINT64 GetLastCompletedFence() = 0;
	virtual void WaitForFence(UINT64 fenceValue) = 0;
};
