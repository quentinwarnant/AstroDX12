#include "FrameResource.h"

#include <d3d12.h>
#include <Rendering/RenderData/RenderConstants.h>

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT renderableObjectCount, UINT computableObjectCount, int16_t frameResourceIndex)
	: m_frameResourceIndex(frameResourceIndex)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAllocator.GetAddressOf())
	));

	PassConstantBuffer = std::make_unique<UploadBuffer<RenderPassConstants>>(device, passCount, true);
	RenderableObjectConstantBuffer = std::make_unique<UploadBuffer<RenderableObjectConstantData>>(device, renderableObjectCount, true);

	constexpr int bufferCountPerComputeObject = 3;
	ComputableObjectStructuredBufferPerObj.reserve(computableObjectCount * bufferCountPerComputeObject);
	for (size_t i = 0; i < computableObjectCount * bufferCountPerComputeObject; ++i)
	{
		ComputeObjectData BufferDataArray
		{
			.Val1{ 42,1}
		};

		std::vector<ComputeObjectData> BufferDataVector { BufferDataArray };
		ComputableObjectStructuredBufferPerObj.push_back(std::make_unique<StructuredBuffer<ComputeObjectData>>(BufferDataVector));
	}
}

FrameResource::~FrameResource()
{
}

D3D12_GPU_VIRTUAL_ADDRESS FrameResource::GetRenderableObjectCbvGpuAddress(size_t ObjectIndex) const
{
	return RenderableObjectPerObjDataCBVgpuAddress[ObjectIndex];
}
