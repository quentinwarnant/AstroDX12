#pragma once

#include <Common.h>
#include <memory>
#include <Rendering/Common/DescriptorHeap.h>

using namespace Microsoft::WRL;

struct RendererContext final
{
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12GraphicsCommandList> CommandList;
    ComPtr<ID3D12CommandQueue> CommandQueue;
    std::weak_ptr<DescriptorHeap> GlobalCBVSRVUAVDescriptorHeap;
};