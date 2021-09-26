#include "RendererDX12.h"
#include "IRenderable.h"

using namespace Microsoft::WRL;
using namespace DX;

RendererDX12::~RendererDX12()
{
	IRenderer::~IRenderer();
}

void RendererDX12::Init(HWND window, int width, int height)
{
#if _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

	m_width = width;
	m_height = height;

	ThrowIfFailed(CreateDXGIFactory2(
#if _DEBUG
		DXGI_CREATE_FACTORY_DEBUG,
#else
		0,
#endif
		IID_PPV_ARGS(&m_dxgiFactory)));

	// Device
	auto deviceCreateResult = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
	if (FAILED(deviceCreateResult))
	{
		//Create WARP device - Windows advanced rasterization platform (software renderer)
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}

	// Fence
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	
	// Descriptor Sizes
	m_descriptorSizeRTV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_descriptorSizeDSV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_descriptorSizeCBV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// MSAA - DISABLED
	//const auto msaaSampleCount = 4;
	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	//msQualityLevels.Format = m_backbufferFormat;
	//msQualityLevels.SampleCount = msaaSampleCount; // 4X MSAA
	//msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	//msQualityLevels.NumQualityLevels = 0;
	//ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
	//m_msaaQualityLevel = msQualityLevels.NumQualityLevels;
	//assert(m_msaaQualityLevel > 0 && "Unexpected MSAA quality level.");

	// Command Queue
	CreateCommandObjects();

	// Swap Chain
	CreateSwapChain(window);
	

}

void RendererDX12::CreateCommandObjects()
{
	auto cmdListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Type = cmdListType;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_commandQueue)));

	ThrowIfFailed(m_device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(m_directCommandListAllocator.GetAddressOf())));

	ThrowIfFailed(m_device->CreateCommandList(
		0,
		cmdListType,
		m_directCommandListAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(m_commandList.GetAddressOf())
	));
	m_commandList->Close();
}

void RendererDX12::CreateSwapChain(HWND window)
{
	m_swapChain.Reset();

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapchainDesc.BufferCount = m_swapChainBufferCount;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapchainDesc.Format = m_backbufferFormat;
	swapchainDesc.Height = m_height;
	swapchainDesc.Width = m_width;
	swapchainDesc.SampleDesc.Count = 1; //msaaSampleCount; NO MSAA, not compatible with this swapchain setup anymore
	swapchainDesc.SampleDesc.Quality = 0;// m_msaaQualityLevel - 1;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.Stereo = false;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapchainFullscreenDesc{};
	swapchainFullscreenDesc.Windowed = TRUE;

	ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		window, 
		&swapchainDesc,
		&swapchainFullscreenDesc,
		nullptr, 
		m_swapChain.GetAddressOf()));

}

void RendererDX12::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.NumDescriptors = m_swapChainBufferCount;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.NodeMask = 0; // device/Adapter index
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc{};
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.NodeMask = 0; // device/Adapter index
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));


}

D3D12_CPU_DESCRIPTOR_HANDLE RendererDX12::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_currentBackBuffer,
		m_descriptorSizeRTV
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE RendererDX12::GetDepthStencilView() const
{
	return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}


void RendererDX12::Render()
{
}

void RendererDX12::AddRenderable(IRenderable* renderable)
{
}

void RendererDX12::FlushRenderQueue()
{
}

void RendererDX12::Shutdown()
{
}

