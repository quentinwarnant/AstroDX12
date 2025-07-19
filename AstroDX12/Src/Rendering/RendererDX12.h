#pragma once

#include <Common.h>
#include <map>
#include <Rendering/Common/DescriptorHeap.h>
#include <Rendering/Common/PipelineStateObjectLibrary.h>
#include <Rendering/IRenderer.h>

using namespace Microsoft::WRL;

class IRenderable;
struct FrameResource;

class RendererDX12 : public IRenderer
{
public:
    virtual ~RendererDX12();

    // IRenderer - BEGIN
    virtual void Init(HWND window, int  width, int height) override;
    virtual void FinaliseInit() override;
    virtual void StartNewFrame(FrameResource* frameResources) override;
    virtual void EndNewFrame(std::function<void(int)> onNewFenceValue) override;

    virtual void ProcessGPUPass(
        const GPUPass& pass,
        const FrameResource& frameResources) override;

    virtual void AddNewFence(std::function<void(int)> onNewFenceValue) override;
    virtual void Shutdown() override;
    virtual void CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature) override;
    virtual RendererContext GetRendererContext() override;

protected:
    virtual ComPtr<ID3D12Device>  GetDevice() const override { return m_device; };
    virtual void CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc) override;
public:
    virtual void Create_const_uav_srv_BufferDescriptorHeaps() override;
    virtual D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferView(D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress, UINT cbvByteSize) override;

    virtual void CreateStructuredBufferAndViews(IStructuredBuffer* structuredBuffer, bool srv, bool uav) override;
    virtual void CreateGraphicsPipelineState(
        ComPtr<ID3D12PipelineState>& pso,
        ComPtr<ID3D12RootSignature>& rootSignature,
        std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
        ComPtr<IDxcBlob>& vertexShaderByteCode,
        ComPtr<IDxcBlob>& pixelShaderByteCode) override;
    virtual void CreateComputePipelineState(
        ComPtr<ID3D12PipelineState>& pso,
        ComPtr<ID3D12RootSignature>& rootSignature,
        std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
        ComPtr<IDxcBlob>& computeShaderByteCode);
    virtual void BuildFrameResources(std::vector<std::unique_ptr<FrameResource>>& outFrameResourcesList, int frameResourcesCount, int renderableObjectCount, int computableObjectCount) override;
    
    virtual void InitialiseRenderTarget(
        std::weak_ptr<RenderTarget> renderTarget,
        LPCWSTR name,
        UINT32 width,
        UINT32 height,
        DXGI_FORMAT format,
        bool initialStateIsUAV = true) override;
    virtual UINT64 GetLastCompletedFence() override;
    virtual void WaitForFence(UINT64 fenceValue) override;
    // IRenderer - END

private:
    void CreateCommandObjects();
    void CreateSwapChain(HWND window);
    void CreateRTVDescriptorHeap();
    void CreateDepthStencilDescriptorHeap();
    void CreateDepthStencilBuffer();

    ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetUAVDescriptorHandleCPU(ERendererProcessedObjectType objectHeapType) const;

private:
    int m_width = 32;
    int m_height = 32;
    static const uint8_t m_swapChainBufferCount = 2;
    int m_currentBackBuffer = 0;
    int m_currentFence = 0;
    size_t m_renderPassCBVOffset = 0;

    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Fence> m_fence;
    ComPtr<IDXGISwapChain1> m_swapChain;

    int32_t m_descriptorSizeRTV;   // Render Target View
    int32_t m_descriptorSizeDSV;   // Depth/Stencil View
    int32_t m_descriptorSizeCBV;   // Constant Buffer View (& SRV/UAV)

    UINT m_msaaQualityLevel;

    DXGI_FORMAT m_backbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_directCommandListAllocator; // Only used during renderer initialisation
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap; // Render Target
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap; // Depth/Stencil 
    std::shared_ptr<DescriptorHeap> m_globalCBVSRVUAVDescriptorHeap; // UAV/SRV/CBV Buffers heap for everything including bindless resources

	int32_t m_rtvHeapViewsCount = 0; // Count of RTV views in the heap

    ComPtr<ID3D12Resource> m_swapchainBuffers[m_swapChainBufferCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;

    D3D12_VIEWPORT m_viewportDesc{};
    D3D12_RECT m_scissorRect{};

    std::unique_ptr<AstroTools::Rendering::PipelineStateObjectLibrary> PSOLibrary;
};

