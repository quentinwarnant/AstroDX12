#pragma once
#include <Rendering/IRenderer.h>
#include <Common.h>
#include <map>

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
    virtual void Render(
        float deltaTime,
        RenderableGroupMap& renderableObjectGroups,
        FrameResource* currentFrameResources,
        std::function<void(int)> onNewFenceValue) override;
    virtual void AddNewFence(std::function<void(int)> onNewFenceValue) override;
    virtual void Shutdown() override;
    virtual void CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature) override;
    virtual void CreateMesh(std::weak_ptr<Mesh>& meshPtr, const void* vertexData, const UINT vertexDataCount, const UINT vertexDataByteSize, const std::vector<std::uint16_t>& indices) override;
protected:
    virtual ComPtr<ID3D12Device>  GetDevice() const override { return m_device; };
public:
    virtual void CreateConstantBufferDescriptorHeaps(int16_t frameResourceCount, int32_t renderableObjectCount) override;
    virtual void CreateConstantBufferView(D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress, UINT cbvByteSize, int32_t handleOffset) override;
    virtual void CreateGraphicsPipelineState(
        ComPtr<ID3D12PipelineState>& pso,
        ComPtr<ID3D12RootSignature>& rootSignature,
        std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
        ComPtr<ID3DBlob>& vertexShaderByteCode,
        ComPtr<ID3DBlob>& pixelShaderByteCode) override;
    virtual void BuildFrameResources(std::vector<std::unique_ptr<FrameResource>>& outFrameResourcesList, int frameResourcesCount, int objectCount) override;
    virtual UINT64 GetLastCompletedFence() override;
    virtual void WaitForFence(UINT64 fenceValue) override;
    virtual void SetPassCBVOffset(int32_t offset) override;
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
        std::unique_ptr<RenderableGroup>& renderablesGroup,
        uint32_t totalRenderables,
        FrameResource* frameResources);

    uint32_t CountTotalRenderables(RenderableGroupMap& renderablesGroupMap);

private:
    int m_width = 32;
    int m_height = 32;
    static const uint8_t m_swapChainBufferCount = 2;
    int m_currentBackBuffer = 0;
    int m_currentFence = 0;
    int32_t m_renderPassCBVOffset = 0;

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
    ComPtr<ID3D12CommandAllocator> m_directCommandListAllocator; // Only used during renderer initialisation
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap; // Render Target
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap; // Depth/Stencil 
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap; // Constant Buffers

    ComPtr<ID3D12Resource> m_swapchainBuffers[m_swapChainBufferCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;

    D3D12_VIEWPORT m_viewportDesc{};
    D3D12_RECT m_scissorRect{};
};

