#pragma once

#include <Common.h>
#include <Rendering/Common/UploadBuffer.h>

using Microsoft::WRL::ComPtr;
struct RenderableObjectConstantData;
struct RenderPassConstants;

struct FrameResource
{
public: 
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
	FrameResource(const FrameResource& other) = delete;
	FrameResource& operator=(const FrameResource& other) = delete;
	~FrameResource();

	// Command allocators - one for each frame, since we can't reset an allocator whilst commands are being processed
	ComPtr<ID3D12CommandAllocator> CmdListAllocator;

	// Each frame needs it's own cbuffers, since one can't be modified whilst being used by another frame
	std::unique_ptr<UploadBuffer<RenderPassConstants>> PassConstantBuffer = nullptr;
	std::unique_ptr<UploadBuffer<RenderableObjectConstantData>> ObjectConstantBuffer = nullptr;

	// Fence value, marking commands up to this fence point. This let's us check if GPU has finished using these frame resources
	UINT64 Fence = 0;
};

