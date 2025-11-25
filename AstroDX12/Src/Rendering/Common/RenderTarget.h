#pragma once

#include <Common.h>

class IRenderer;
class DescriptorHeap;

using Microsoft::WRL::ComPtr;

class RenderTarget
{
public:
    RenderTarget()
		:   m_renderTargetResource(nullptr),
            m_width(0),
            m_height(0),
            m_format(DXGI_FORMAT_UNKNOWN),
		    m_uavIndex(-1),
		    m_srvIndex(-1)
    {
    }

    virtual ~RenderTarget()
    {
        ReleaseResources();
    }

    void Initialize(
        IRenderer* renderer,
        DescriptorHeap& descriptorHeap, 
        LPCWSTR name,
        UINT32 width,
        UINT32 height,
        DXGI_FORMAT format,
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_RENDER_TARGET);

    void ReleaseResources();

	int32_t GetUAVIndex() const
    {
        return m_uavIndex; 
    }
    
    int32_t GetSRVIndex() const
    {
        return m_srvIndex; 
	}

    ID3D12Resource* GetResource() const
    {
        return m_renderTargetResource.Get();
	}

    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle() const
    {
        return m_srvDescriptorHandle;
	}

    D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGPUDescriptorHandle() const
    {
        return m_uavDescriptorHandle;
	}

    D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCPUDescriptorHandle() const
    {
        return m_uavCPUDescriptorHandle;
	}

private:
    ComPtr<ID3D12Resource> m_renderTargetResource = nullptr;
    UINT32 m_width = 0;
    UINT32 m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_UNKNOWN;
    int32_t m_uavIndex = -1;
	int32_t m_srvIndex = -1;
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvDescriptorHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_uavDescriptorHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_uavCPUDescriptorHandle = {};
};

