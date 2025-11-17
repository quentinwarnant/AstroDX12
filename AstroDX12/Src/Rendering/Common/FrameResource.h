#pragma once

#include <Common.h>
#include <Rendering/Common/UploadBuffer.h>
#include <Rendering/Common/StructuredBuffer.h>

using Microsoft::WRL::ComPtr;
struct RenderableObjectConstantData;
struct RenderPassConstants;

#include <GameContent/Compute/ComputeObjectConstantData.h>

struct FrameResource final
{
public: 
	FrameResource(ID3D12Device* device, UINT passCount, int16_t frameResourceIndex);
	FrameResource(const FrameResource& other) = delete;
	FrameResource& operator=(const FrameResource& other) = delete;
	virtual ~FrameResource();

	int16_t GetIndex() { return m_frameResourceIndex; }

	// Command allocators - one for each frame, since we can't reset an allocator whilst commands are being processed
	ComPtr<ID3D12CommandAllocator> CmdListAllocator;

	// Each frame needs it's own cbuffers, since one can't be modified whilst being used by another frame
	std::unique_ptr<UploadBuffer<RenderPassConstants>> PassConstantBuffer = nullptr;
	
	// Fence value, marking commands up to this fence point. This let's us check if GPU has finished using these frame resources
	UINT64 Fence = 0;

private:
	int16_t m_frameResourceIndex = -1;
};

