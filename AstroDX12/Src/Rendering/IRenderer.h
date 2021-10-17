#pragma once

#include "../Common.h"

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
		ComPtr<ID3D12PipelineState>& pipelineStateObj,
		ComPtr<ID3D12DescriptorHeap>& constBufferViewHeap) = 0;
	virtual void FlushRenderQueue() = 0;
	virtual void Shutdown() = 0;
};

