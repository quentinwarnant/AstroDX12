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
    virtual void Init(HWND window, int  width, int height) override;
    virtual void FinaliseInit() override;
    virtual void Render(
        float deltaTime,
        std::vector< std::shared_ptr<IRenderable>>& renderableObjects,
        ComPtr<ID3D12PipelineState>& pipelineStateObj) override;
    virtual void FlushRenderQueue() override;
    virtual void Shutdown() override;
    virtual void CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature) override;

protected:
    virtual ComPtr<ID3D12Device>  GetDevice() const override { return m_device; };
public:
    virtual void CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc) override;
    // IRenderer - END

private:
    void CreateCommandObjects();
    void CreateSwapChain(HWND window);
    void CreateDescriptorHeaps();
    void CreateDepthStencilBuffer();

    ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    void ProcessRenderableObjectsForRendering(
        ComPtr<ID3D12GraphicsCommandList>& commandList,
        std::vector<std::shared_ptr<IRenderable>>& renderableObjects);

private:
    int m_width = 32;
    int m_height = 32;
    static const uint8_t m_swapChainBufferCount = 2;
    int m_currentBackBuffer = 0;
    int m_currentFence = 0;

    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Fence> m_fence;
    ComPtr<IDXGISwapChain1> m_swapChain;

    UINT m_descriptorSizeRTV;   // Render Target View
    UINT m_descriptorSizeDSV;   // Depth/Stencil View
    UINT m_descriptorSizeCBV;   // Constant Buffer View (& SRV/UAV)

    UINT m_msaaQualityLevel;

    DXGI_FORMAT m_backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_directCommandListAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap; // Render Target
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap; // Depth/Stencil 
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap; // Constant Buffers

    ComPtr<ID3D12Resource> m_swapchainBuffers[m_swapChainBufferCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;

    D3D12_VIEWPORT m_viewportDesc{};
    D3D12_RECT m_scissorRect{};

};

