#include "RendererDX12.h"
#include "IRenderable.h"

using namespace Microsoft::WRL;
using namespace DX;

RendererDX12::~RendererDX12()
{
	IRenderer::~IRenderer();
}

void RendererDX12::Init(int width, int height)
{
#if _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif

	m_width = width;
	m_height = height;

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

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

	// MSAA
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = m_backbufferFormat;
	msQualityLevels.SampleCount = 4; // 4X MSAA
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
	m_msaaQualityLevel = msQualityLevels.NumQualityLevels;
	assert(m_msaaQualityLevel > 0 && "Unexpected MSAA quality level.");

	// Command Queue
	CreateCommandObjects();
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

