#include "FrameResource.h"

#include <d3d12.h>
#include <Rendering/RenderData/RenderConstants.h>

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, int16_t frameResourceIndex)
	: m_frameResourceIndex(frameResourceIndex)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAllocator.GetAddressOf())
	));

	PassConstantBuffer = std::make_unique<UploadBuffer<RenderPassConstants>>(device, passCount, true);

}

FrameResource::~FrameResource()
{
}
