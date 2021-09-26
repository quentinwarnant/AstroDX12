#pragma once
#include "IRenderer.h"
#include "../Common.h"

using namespace Microsoft::WRL;

class IRenderable;

class RendererDX12 : public IRenderer
{
public:
    virtual ~RendererDX12();

    // IRenderer - BEGIN
    virtual void Init(int  width, int height) override;
    virtual void Render() override;
    virtual void AddRenderable(IRenderable* renderable) override;
    virtual void FlushRenderQueue() override;
    virtual void Shutdown() override;
    // IRenderer - END
private:
    void CreateCommandObjects();


private:
    int m_width = 32;
    int m_height = 32;

    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Fence> m_fence;

    UINT m_descriptorSizeRTV;   // Render Target View
    UINT m_descriptorSizeDSV;   // Depth/Stencil View
    UINT m_descriptorSizeCBV;   // Constant Buffer View (& SRV/UAV)

    UINT m_msaaQualityLevel;

    DXGI_FORMAT m_backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_directCommandListAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
};

