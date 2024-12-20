#pragma once
#include <Rendering/IRenderer.h>
#include <Common.h>
#include <map>

#include <Rendering/Common/PipelineStateObjectLibrary.h>

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
        const RenderableGroupMap& renderableObjectGroups,
        const ComputeGroup& computeObjectGroup,
        FrameResource* currentFrameResources,
        std::function<void(int)> onNewFenceValue) override;
    virtual void AddNewFence(std::function<void(int)> onNewFenceValue) override;
    virtual void Shutdown() override;
    virtual void CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature) override;
    virtual void AllocateMeshBackingBuffers(
        std::weak_ptr<Mesh>& meshPtr, 
        const void* vertexData, 
        const UINT vertexDataCount, 
        const UINT vertexDataByteSize, 
        const std::vector<std::uint16_t>& indices) override;
protected:
    virtual ComPtr<ID3D12Device>  GetDevice() const override { return m_device; };
public:
    virtual void Create_const_uav_srv_BufferDescriptorHeaps(size_t frameResourceCount, size_t renderableObjectCount, size_t computableObjectCount) override;
    virtual void CreateConstantBufferView(D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress, UINT cbvByteSize, size_t handleOffset) override;

    virtual void CreateStructuredBufferAndViews(IStructuredBuffer* structuredBuffer, bool srv, bool uav, size_t handleOffset) override;
    virtual void CreateGraphicsPipelineState(
        ComPtr<ID3D12PipelineState>& pso,
        ComPtr<ID3D12RootSignature>& rootSignature,
        std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
        ComPtr<ID3DBlob>& vertexShaderByteCode,
        ComPtr<ID3DBlob>& pixelShaderByteCode) override;
    virtual void CreateComputePipelineState(
        ComPtr<ID3D12PipelineState>& pso,
        ComPtr<ID3D12RootSignature>& rootSignature,
        std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
        ComPtr<ID3DBlob>& computeShaderByteCode);
    virtual void BuildFrameResources(std::vector<std::unique_ptr<FrameResource>>& outFrameResourcesList, int frameResourcesCount, int renderableObjectCount, int computableObjectCount) override;
    virtual UINT64 GetLastCompletedFence() override;
    virtual void WaitForFence(UINT64 fenceValue) override;
    virtual void SetPassCBVOffset(size_t offset) override;
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

    void ProcessRenderableObjectsForRendering(
        ComPtr<ID3D12GraphicsCommandList>& commandList,
        const std::unique_ptr<RenderableGroup>& renderablesGroup,
        FrameResource* frameResources);
    uint32_t CountTotalRenderables(const RenderableGroupMap& renderablesGroupMap);

    void ProcessComputableObjects(
        ComPtr<ID3D12GraphicsCommandList>& commandList,
        const ComputeGroup& computeObjectGroup,
        uint32_t totalComputables,
        FrameResource* frameResources);

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
    ComPtr<ID3D12DescriptorHeap> m_renderableObjectCBVSRVUAVHeap; // Constant Buffers heap for renderables
    ComPtr<ID3D12DescriptorHeap> m_computableObjectSRVUAVHeap; // AUV/SRV/CBV Buffers heap for computables


    ComPtr<ID3D12Resource> m_swapchainBuffers[m_swapChainBufferCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;

    D3D12_VIEWPORT m_viewportDesc{};
    D3D12_RECT m_scissorRect{};

    std::unique_ptr<AstroTools::Rendering::PipelineStateObjectLibrary> PSOLibrary;
};

