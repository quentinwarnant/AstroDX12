#include <Rendering/RendererDX12.h>
#include <Rendering/Common/FrameResource.h>
#include <Rendering/Common/RendererContext.h>
#include <Rendering/Common/StructuredBuffer.h>
#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/Compute/ComputeGroup.h>
#include <Rendering/Common/GPUPass.h>
#include <Rendering/Common/SamplerIDs.h>

using namespace Microsoft::WRL;
using namespace DX;

RendererDX12::~RendererDX12()
{
	IRenderer::~IRenderer();
}

void RendererDX12::Init(HWND window, int width, int height)
{
#if _DEBUG
	ComPtr<ID3D12Debug1> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
	debugController->SetEnableGPUBasedValidation(TRUE);
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
	
	// Collect adapters, and select the best one
	UINT AdaptedIndex = 0;
	IDXGIAdapter* currentAdapter;
	{

		std::vector <IDXGIAdapter*> availableAdapters;
		while (m_dxgiFactory->EnumAdapters(AdaptedIndex, &currentAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			availableAdapters.push_back(currentAdapter);
			++AdaptedIndex;
		}

		// -> Make sure to select the best gpu available
		// not a great method but let's pick the one with the most vram instead of fiddling with vendorIDs or device IDs 
		if (availableAdapters.size() > 1)
		{
			size_t maxVRAM = 0;
			size_t adapterWithMaxVRAMIndex = 0;
			for(size_t index = 0; index < availableAdapters.size(); ++index)
			{
				const auto& adapter = availableAdapters[index];
				DXGI_ADAPTER_DESC adapterDesc{};
				adapter->GetDesc(&adapterDesc);
				if (adapterDesc.DedicatedVideoMemory > maxVRAM)
				{
					maxVRAM = adapterDesc.DedicatedVideoMemory;
					adapterWithMaxVRAMIndex = index;
				}
			}
			currentAdapter = availableAdapters[adapterWithMaxVRAMIndex];
		}
		else
		{
			currentAdapter = availableAdapters[0];
		}
	}

	
	auto deviceCreateResult = D3D12CreateDevice(currentAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device));
	if (FAILED(deviceCreateResult))
	{
		//Create WARP device - Windows advanced rasterization platform (software renderer)
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}


	//D3D12_FEATURE_DATA_D3D12_OPTIONS featureSupport{};
	//if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureSupport, sizeof(featureSupport)) ) )
	//{
	//	assert(false);
	//}
	//else
	//{

	//	switch (featureSupport.ResourceBindingTier)
	//	{
	//	case D3D12_RESOURCE_BINDING_TIER_1:
	//		// Tier 1 is supported.
	//		break;

	//	case D3D12_RESOURCE_BINDING_TIER_2:
	//		// Tiers 1 and 2 are supported.
	//		break;

	//	case D3D12_RESOURCE_BINDING_TIER_3:
	//		// Tiers 1, 2, and 3 are supported.
	//		break;
	//	}

	//}

	//
	D3D12_FEATURE_DATA_SHADER_MODEL  shaderModelSupport{};
	shaderModelSupport.HighestShaderModel = D3D_SHADER_MODEL_6_7;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelSupport, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL))))
	{
		assert(false);
		DX::astro_assert(false, "D3D12 device doesn't support shader model 6.7 which is necessary for bindless rendering. This renderer won't work for this GPU");
	}
	else
	{
		switch (shaderModelSupport.HighestShaderModel)
		{
		case D3D_SHADER_MODEL_6_6: 
			break;

		default:
			break;
		
		}
	}

	HMODULE a = LoadLibraryA("D3D12Core.dll");
	char path[500];
	GetModuleFileNameA(a, path, 500);


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

	CreateGlobalDescriptorHeaps();
	CreateDefaultGlobalSamplers();

	m_rendererContext = {
		.Device = m_device,
		.CommandList = m_commandList,
		.GlobalCBVSRVUAVDescriptorHeap = m_globalCBVSRVUAVDescriptorHeap
	};

	// Swap Chain
	CreateSwapChain(window);

	CreateRTVDescriptorHeap();
	CreateDepthStencilDescriptorHeap();

	// Backbuffer Render Target
	for (auto i = 0; i < m_swapChainBufferCount; ++i)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapchainBuffers[i])));

		CreateRenderTargetView(m_swapchainBuffers[i].Get(), nullptr);
	}

	ThrowIfFailed(m_commandList->Reset(m_directCommandListAllocator.Get(), nullptr));

	// Depth/Stencil Buffer
	CreateDepthStencilBuffer();

	// Create Viewport
	m_viewportDesc.TopLeftX = 0;
	m_viewportDesc.TopLeftY = 0;
	m_viewportDesc.Width = static_cast<float>(m_width);
	m_viewportDesc.Height = static_cast<float>(m_height);
	m_viewportDesc.MinDepth = 0;
	m_viewportDesc.MaxDepth = 1;
	m_commandList->RSSetViewports(1, &m_viewportDesc);

	// Create Scissor Rect (pixels outside are culled/not rasterised)

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_width;
	m_scissorRect.bottom = m_height;
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Create Pipeline State Object Library for caching
	PSOLibrary = std::make_unique<AstroTools::Rendering::PipelineStateObjectLibrary>();

	m_dummyTex = std::make_unique<RenderTarget>();
	InitialiseRenderTarget(m_dummyTex.get(), L"DummyTex2D", 1, 1, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void RendererDX12::FinaliseInit()
{
	// Execute 
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	AddNewFence([](int) {});
}

void RendererDX12::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHeapHandle.Offset(m_rtvHeapViewsCount, m_descriptorSizeRTV);

	//Create RT view for buffer
	m_device->CreateRenderTargetView(resource, desc, rtvHeapHandle);
	m_rtvHeapViewsCount++;
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

void RendererDX12::CreateRTVDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = m_swapChainBufferCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0; // device/Adapter index
	ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));

	m_rtvHeapViewsCount = 0;
}

void RendererDX12::CreateDepthStencilDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0; // device/Adapter index
	ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));
}

void RendererDX12::CreateGlobalDescriptorHeaps()
{
	// Let's just make a bunch!
	size_t descriptorCount = 10'000u;

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = (UINT)descriptorCount;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0; // device/Adapter index

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderablesCBVSRVUAVHeap;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(renderablesCBVSRVUAVHeap.GetAddressOf())));
	m_globalCBVSRVUAVDescriptorHeap = std::make_shared<DescriptorHeap>( renderablesCBVSRVUAVHeap, m_descriptorSizeCBV );

	// ----------------------------------------

	D3D12_DESCRIPTOR_HEAP_DESC cpuDescriptorHeapDesc{};
	cpuDescriptorHeapDesc.NumDescriptors = (UINT)descriptorCount;
	cpuDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cpuDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cpuDescriptorHeapDesc.NodeMask = 0; // device/Adapter index

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cpuRTVHeap;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cpuDescriptorHeapDesc, IID_PPV_ARGS(cpuRTVHeap.GetAddressOf())));
	m_cpuGlobalUAVDescriptorHeap = std::make_shared<DescriptorHeap>(cpuRTVHeap, m_descriptorSizeCBV);

	// ----------------------------------------

	size_t samplerDescriptorCount = 1000u;
	D3D12_DESCRIPTOR_HEAP_DESC samplerDescriptorHeapDesc{};
	samplerDescriptorHeapDesc.NumDescriptors = (UINT)samplerDescriptorCount;
	samplerDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	samplerDescriptorHeapDesc.NodeMask = 0; // device/Adapter index
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerHeap;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerDescriptorHeapDesc, IID_PPV_ARGS(samplerHeap.GetAddressOf())));
	m_globalSamplerDescriptorHeap = std::make_shared<DescriptorHeap>(samplerHeap, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));

}

void RendererDX12::CreateDepthStencilBuffer()
{
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = m_width;
	depthStencilDesc.Height = m_height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = m_depthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1; // NO MSAA, not compatible with this swapchain setup anymore
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = m_depthStencilFormat;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;
	
	const auto depthStencilHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_device->CreateCommittedResource(
		&depthStencilHeapProp, // Default heap (others: upload, readback & custom
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&depthClearValue,
		IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())
	));

	// Create Depth/Stencil buffer View
	m_device->CreateDepthStencilView(
		m_depthStencilBuffer.Get(),
		nullptr, // Buffer is typed so no need to define it here
		GetDepthStencilView()
	);

	// Transition resource from initial state to be used as depth buffer
	const auto depthStencilStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_commandList->ResourceBarrier(
		1,
		&depthStencilStateTransition
	);
}

ComPtr<ID3D12Resource> RendererDX12::GetCurrentBackBuffer() const
{
	return m_swapchainBuffers[m_currentBackBuffer];
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

D3D12_CPU_DESCRIPTOR_HANDLE RendererDX12::GetUAVDescriptorHandleCPU() const
{
	// UAV & SRV are created on the same heap as CBV since they're the same size
	return m_globalCBVSRVUAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void RendererDX12::CreateRootSignature(ComPtr<ID3DBlob>& serializedRootSignature, ComPtr<ID3D12RootSignature>& outRootSignature)
{
	ThrowIfFailed( m_device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&outRootSignature)) );
}

void RendererDX12::StartNewFrame( FrameResource* frameResources )
{
	// We know at this point we've waited for last frame's commands to be executed on the GPU , we can now safely reset the commandlist allocator
	ThrowIfFailed(frameResources->CmdListAllocator->Reset());

	// Reset command list so we can re-use it for the next frame worth of commands.
	// This is safe to do after the commandlist is comitted to the command queue with ExecuteCommandList
	ThrowIfFailed(m_commandList->Reset(frameResources->CmdListAllocator.Get(), nullptr));

	// Switch back buffer render target buffer from Presenting to Render Target mode
	const auto backBufferResourceBarrierPresentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer().Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_commandList->ResourceBarrier(
		1,
		&backBufferResourceBarrierPresentToRT
	);

	// Always need to re-set the viewport & scissor rect after resetting the command list
	m_commandList->RSSetViewports(1, &m_viewportDesc);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// Clear the Backbuffer & depth buffer
	constexpr FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const auto currentBackBufferView = GetCurrentBackBufferView();
	const auto currentDepthStencilView = GetDepthStencilView();
	m_commandList->ClearRenderTargetView(currentBackBufferView, ClearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(currentDepthStencilView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set buffer we're rendering to (output merger)
	m_commandList->OMSetRenderTargets(1, &currentBackBufferView, true, &currentDepthStencilView);

	ID3D12DescriptorHeap* globalDescriptorHeaps[] =
	{
		m_globalCBVSRVUAVDescriptorHeap->GetHeapPtr(),
		m_globalSamplerDescriptorHeap->GetHeapPtr(),
	};
	m_commandList->SetDescriptorHeaps(_countof(globalDescriptorHeaps), globalDescriptorHeaps);

}

void RendererDX12::EndNewFrame(std::function<void(int)> onNewFenceValue)
{
	// Transition backbuffer resource state from render target to present 
	const auto backBufferResourceBarrierRTToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_commandList->ResourceBarrier(
		1,
		&backBufferResourceBarrierRTToPresent
	);

	ThrowIfFailed(m_commandList->Close());

	// add command lists on queue
	ID3D12CommandList* cmdLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// Present swaps the back & front buffers
	ThrowIfFailed(m_swapChain->Present(0, 0));
	m_currentBackBuffer = (m_currentBackBuffer + 1) % m_swapChainBufferCount;

	AddNewFence(onNewFenceValue);
}

void RendererDX12::ProcessGPUPass(
	const GPUPass& pass,
	const FrameResource& frameResources,
	float deltaTime)
{
	pass.Execute(m_commandList, deltaTime, frameResources);
}

void RendererDX12::AddNewFence(std::function<void(int)> onNewFenceValue)
{
	// Advance current fence
	m_currentFence++;
	onNewFenceValue(m_currentFence);

	// Add fence on GPU queue which will get signalled when queue is fully processed
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFence));
}

void RendererDX12::Shutdown()
{
	ComPtr<ID3D12DebugDevice> debugDevice;
	if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice)))) {
		// You now have a debug device
		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_FLAGS::D3D12_RLDO_DETAIL);
	}

}

D3D12_GPU_DESCRIPTOR_HANDLE RendererDX12::CreateConstantBufferView(D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress, UINT cbvByteSize)
{
	const auto cbvDescriptorIndex = m_globalCBVSRVUAVDescriptorHeap->GetCurrentDescriptorHeapHandle();
	const auto cpuDescriptorHandle = m_globalCBVSRVUAVDescriptorHeap->GetCPUDescriptorHandleByIndex(cbvDescriptorIndex);
	const auto gpuDescriptorHandle = m_globalCBVSRVUAVDescriptorHeap->GetGPUDescriptorHandleByIndex(cbvDescriptorIndex);
	m_globalCBVSRVUAVDescriptorHeap->IncreaseCurrentDescriptorHeapHandle();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
	cbViewDesc.BufferLocation = cbvGpuAddress;
	cbViewDesc.SizeInBytes = cbvByteSize;

	m_device->CreateConstantBufferView(&cbViewDesc, cpuDescriptorHandle);

	return gpuDescriptorHandle;
}

void RendererDX12::CreateStructuredBufferAndViews(IStructuredBuffer* structuredBuffer, bool srv, bool uav)
{
	structuredBuffer->Init(m_device.Get(), m_commandList.Get(), srv, uav, *m_globalCBVSRVUAVDescriptorHeap);
}

void RendererDX12::CreateGraphicsPipelineState(
	ComPtr<ID3D12PipelineState>& pso,
	ComPtr<ID3D12RootSignature>& rootSignature,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>* inputLayout,
	ComPtr<IDxcBlob>& vertexShaderByteCode,
	ComPtr<IDxcBlob>& pixelShaderByteCode,
	bool wireframeEnabled /*= false*/)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	if( inputLayout == nullptr )
	{
		// If input layout is not provided, we assume bindless rendering and don't set it
		psoDesc.InputLayout = { nullptr, 0 };
	}
	else
	{
		// Otherwise we use the provided input layout
		psoDesc.InputLayout = { inputLayout->data(), (UINT)inputLayout->size() };
	}
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vertexShaderByteCode->GetBufferPointer()),
		vertexShaderByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(pixelShaderByteCode->GetBufferPointer()),
		pixelShaderByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = wireframeEnabled ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] =  m_backbufferFormat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = m_depthStencilFormat;

	// Either retrieve PSO from cached library if identical one was already produced, or create a new one and cache it now
	if (auto cachedPSO = PSOLibrary->GetPSO(psoDesc))
	{
		pso = cachedPSO;
	}
	else
	{
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		PSOLibrary->CachePSO(psoDesc, pso);
	}
}

void RendererDX12::CreateComputePipelineState(
	ComPtr<ID3D12PipelineState>& pso,
	ComPtr<ID3D12RootSignature>& rootSignature,
	ComPtr<IDxcBlob>& computeShaderByteCode)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.CS = 
	{
		reinterpret_cast<BYTE*>(computeShaderByteCode->GetBufferPointer()),
		computeShaderByteCode->GetBufferSize()
	};
	psoDesc.NodeMask = 0; // GPU node index (eg if there were multiple GPUs)
	
	D3D12_CACHED_PIPELINE_STATE cachedPSOobj;
	cachedPSOobj.CachedBlobSizeInBytes = 0;
	cachedPSOobj.pCachedBlob = NULL;
	psoDesc.CachedPSO = cachedPSOobj;

	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// Either retrieve PSO from cached library if identical one was already produced, or create a new one and cache it now
	if (auto cachedPSO = PSOLibrary->GetPSO(psoDesc))
	{
		pso = cachedPSO;
	}
	else
	{
		ThrowIfFailed(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		PSOLibrary->CachePSO(psoDesc, pso);
	}
}

void RendererDX12::BuildFrameResources(std::vector<std::unique_ptr<FrameResource>>& outFrameResourcesList, int frameResourcesCount)
{
	for (int16_t i = 0; i < frameResourcesCount; ++i)
	{
		outFrameResourcesList.push_back(std::make_unique<FrameResource>(
			m_device.Get(),
			1,
			i
			));
	}
}

void RendererDX12::InitialiseRenderTarget(
	RenderTarget* renderTarget,
	LPCWSTR name,
	UINT32 width,
	UINT32 height,
	DXGI_FORMAT format,
	D3D12_RESOURCE_STATES initialState)
{
	renderTarget->Initialize(this, *m_globalCBVSRVUAVDescriptorHeap.get(), *m_cpuGlobalUAVDescriptorHeap.get(), name, width, height, format, initialState);
}

RendererContext& RendererDX12::GetRendererContext()
{
	return m_rendererContext;
}

UINT64 RendererDX12::GetLastCompletedFence()
{
	return m_fence->GetCompletedValue();
}

void RendererDX12::WaitForFence(UINT64 fenceValue)
{
	HANDLE eventHandle = CreateEventW(nullptr, false, false, nullptr);
	ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, eventHandle));
	WaitForSingleObject(eventHandle, INFINITE);
	CloseHandle(eventHandle);
}

void RendererDX12::CreateDefaultGlobalSamplers()
{
	// 1. AstroTools::Rendering::SamplerIDs::LinearClamp
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.BorderColor[0] = samplerDesc.BorderColor[1] = samplerDesc.BorderColor[2] = samplerDesc.BorderColor[3] = 0.0f;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_globalSamplerDescriptorHeap->GetCPUDescriptorHandleByIndex(m_globalSamplerDescriptorHeap->GetCurrentDescriptorHeapHandle());
	m_device->CreateSampler(&samplerDesc, cpuHandle);
	m_globalSamplerDescriptorHeap->IncreaseCurrentDescriptorHeapHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE RendererDX12::GetSamplerGPUHandle(int32_t samplerID)
{
	DX::astro_assert(samplerID < m_globalSamplerDescriptorHeap->GetCurrentDescriptorHeapHandle(), "Requested sampler ID is out of bounds");

	return m_globalSamplerDescriptorHeap->GetGPUDescriptorHandleByIndex(samplerID);
	
}

D3D12_GPU_DESCRIPTOR_HANDLE RendererDX12::GetDummySRVGPUHandle() const
{
	return m_dummyTex->GetSRVGPUDescriptorHandle();
}
