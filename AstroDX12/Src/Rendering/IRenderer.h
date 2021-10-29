#pragma once

#include "../Common.h"
#include "Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;

class IRenderable;

class IRenderer
{
public:
	virtual ~IRenderer() {};
	virtual void Init(HWND window, int width, int height) = 0;
	virtual void FinaliseInit() = 0;
	virtual void Render(
		float deltaTime,
		std::vector<std::shared_ptr<IRenderable>>& renderableObjects,
		ComPtr<ID3D12PipelineState>& pipelineStateObj) = 0;
	virtual void FlushRenderQueue() = 0;
	virtual void Shutdown() = 0;

protected:
	virtual ComPtr<ID3D12Device> GetDevice() const = 0;

public:
	template<typename T>
	std::unique_ptr<UploadBuffer<T>> CreateConstantBuffer(UINT elementCount)
	{
		return std::make_unique<UploadBuffer<T>>(GetDevice().Get(), elementCount, true);
	}

	virtual void CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc) = 0;
};
