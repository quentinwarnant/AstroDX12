#pragma once

#include <Common.h>
#include <Rendering/Common/DescriptorHeap.h>

using namespace Microsoft::WRL;

struct RendererContext
{
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12GraphicsCommandList> CommandList;
    DescriptorHeap* RenderableObjectCBVSRVUAVHeap;
};