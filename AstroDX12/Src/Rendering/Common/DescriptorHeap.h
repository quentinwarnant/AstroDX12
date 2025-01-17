#pragma once
#include <Common.h>

using namespace Microsoft::WRL;

// A helpful wrapper around a resource description heap object
class DescriptorHeap
{
public:

    DescriptorHeap()
        : m_heap(nullptr)
        , m_descriptorSize(0)
        , m_currentDescriptorHeapHandle(0)
    { }

    DescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap, int32_t descriptorSize)
        : m_heap(heap)
        , m_descriptorSize(descriptorSize)
        , m_currentDescriptorHeapHandle(0)
    {
    }

    int32_t GetCurrentDescriptorHeapHandle() const
    {
        return m_currentDescriptorHeapHandle;
    }

    void IncreaseCurrentDescriptorHeapHandle()
    {
        m_currentDescriptorHeapHandle++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const
    {
        return m_heap->GetCPUDescriptorHandleForHeapStart();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const
    {
        return m_heap->GetGPUDescriptorHandleForHeapStart();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleByIndex(size_t Index)
    {
        auto descriptorHandle = GetCPUDescriptorHandleForHeapStart();
        descriptorHandle.ptr += m_descriptorSize *  Index;// static_cast<unsigned long long>(Index);
        return descriptorHandle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleByIndex(size_t Index)
    {
        auto descriptorHandle = GetGPUDescriptorHandleForHeapStart();
        descriptorHandle.ptr += m_descriptorSize * Index;// static_cast<unsigned long long>(Index);
        return descriptorHandle;
    }

    ID3D12DescriptorHeap* GetHeapPtr() const
    {
        return m_heap.Get();
    }

    UINT GetDescriptorSize() const
    {
        return (UINT)m_descriptorSize;
    }

private:

    ComPtr<ID3D12DescriptorHeap> m_heap { nullptr }; // eg: CBV/UAV/SRV heap for renderables
    int32_t m_descriptorSize{ 0 };
    int32_t m_currentDescriptorHeapHandle {0};
};