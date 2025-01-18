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
	FrameResource(ID3D12Device* device, UINT passCount, UINT renderableObjectCount, UINT computableObjectCount, int16_t frameResourceIndex);
	FrameResource(const FrameResource& other) = delete;
	FrameResource& operator=(const FrameResource& other) = delete;
	virtual ~FrameResource();

	int16_t GetIndex() { return m_frameResourceIndex; }

	// Command allocators - one for each frame, since we can't reset an allocator whilst commands are being processed
	ComPtr<ID3D12CommandAllocator> CmdListAllocator;

	// Each frame needs it's own cbuffers, since one can't be modified whilst being used by another frame
	std::unique_ptr<UploadBuffer<RenderPassConstants>> PassConstantBuffer = nullptr;
	
	//TODO: remove this in favour of bindless lookup
	std::unique_ptr<UploadBuffer<RenderableObjectConstantData>> RenderableObjectConstantBuffer = nullptr;
	std::vector< D3D12_GPU_VIRTUAL_ADDRESS> RenderableObjectPerObjDataCBVgpuAddress;
	

	// Each compute object has it's own structured buffer, in this array, use their index to access the right buffer.
	std::vector<std::unique_ptr<StructuredBuffer<ComputeObjectData>>> ComputableObjectStructuredBufferPerObj;
	std::vector< D3D12_GPU_VIRTUAL_ADDRESS> 	ComputableObjectPerObjDataCBVgpuAddress; // unused - gpu address is directly accessed from the buffer object

	// Fence value, marking commands up to this fence point. This let's us check if GPU has finished using these frame resources
	UINT64 Fence = 0;

	D3D12_GPU_VIRTUAL_ADDRESS GetRenderableObjectCbvGpuAddress(size_t ObjectIndex) const;

private:
	int16_t m_frameResourceIndex = -1;
};

