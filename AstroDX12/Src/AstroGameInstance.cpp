#include <AstroGameInstance.h>

#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/RenderData/VertexDataFactory.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/VertexDataInputLayoutLibrary.h>
#include <Rendering/Compute/ComputableObject.h>
#include <Rendering/Common/VectorTypes.h>
#include <Rendering/CommonMeshes.h>


#include <GameContent/GPUPasses/BasePassSceneGeometry.h>
#include <GameContent/GPUPasses/Compute/ComputePassParticles.h>
#include <GameContent/GPUPasses/Compute/ComputePassRaymarchScene.h>
#include <GameContent/GPUPasses/GraphicsPassCopyGBufferToBackbuffer.h>
#include <GameContent/GPUPasses/Compute/ComputePassPhysicsChain.h>
#include <GameContent/GPUPasses/Debugging/GraphicsPassDebugDraw.h>
#include <GameContent/GPUPasses/Debugging/ComputePassVertexLineDebugDraw.h>
#include <GameContent/GPUPasses/Compute/ComputePassFluidSim2D.h>
#include <GameContent/GPUPasses/Compute/ComputePassPicFlip3D.h>

#include <Rendering/RenderData/RenderConstants.h>

#include <GameContent/Scene/SceneLoader.h>

namespace
{
	namespace
	{
		constexpr size_t NumFrameResources = 3;
	}
}

AstroGameInstance::AstroGameInstance()
	: Game()
	, m_frameIdx(0)
	, m_cameraPos(0,0,0)
	, m_frameResources()
	, m_currentFrameResource(nullptr)
	, m_currentFrameResourceIndex(0)
	, m_meshLibrary(std::make_unique<MeshLibrary>())
	, m_gpuPasses()
{
}

AstroGameInstance::~AstroGameInstance()
{
}

void AstroGameInstance::InitCamera()
{
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_projMat, proj);
}

void AstroGameInstance::CreateConstantBufferViews()
{
	// Create CBV descriptors shared for the whole render pass, in each frame resource, after all the renderable objects in the renderable object heap
	for (int16_t frameIdx = 0; frameIdx < NumFrameResources; ++frameIdx)
	{
		UINT passCBByteSize = m_frameResources[frameIdx]->PassConstantBuffer->GetElementByteSize();
		auto passCB = m_frameResources[frameIdx]->PassConstantBuffer->Resource();

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Finalise creation of constant buffer view
		const auto cbvHeapDescriptorIndex = m_renderer->CreateConstantBufferView(cbAddress, passCBByteSize);
		m_frameResources[frameIdx]->PassConstantBuffer->SetHeapDescriptorIndex(cbvHeapDescriptorIndex);
	}
}

void AstroGameInstance::BuildFrameResources()
{
	m_renderer->BuildFrameResources(m_frameResources, NumFrameResources);
}

void AstroGameInstance::UpdateFrameResource()
{
	// cycle through the Circular buffer
	m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % NumFrameResources;
	m_currentFrameResource = m_frameResources[m_currentFrameResourceIndex].get();

	if (m_currentFrameResource->Fence != 0 &&
		m_renderer->GetLastCompletedFence() < m_currentFrameResource->Fence)
	{
		m_renderer->WaitForFence(m_currentFrameResource->Fence);
	}
}

void AstroGameInstance::UpdateMainRenderPassConstantBuffer(float deltaTime)
{
	XMMATRIX view = XMLoadFloat4x4(&m_viewMat);
	XMMATRIX proj = XMLoadFloat4x4(&m_projMat);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	auto viewDet = XMMatrixDeterminant(view);
	XMMATRIX invView = XMMatrixInverse(&viewDet, view);
	auto projDet = XMMatrixDeterminant(proj);
	XMMATRIX invProj = XMMatrixInverse(&projDet, proj);
	auto viewProjDet = XMMatrixDeterminant(viewProj);
	XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	RenderPassConstants renderPassCB;
	XMStoreFloat4x4(&renderPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&renderPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&renderPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&renderPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&renderPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&renderPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	renderPassCB.EyePosWorld = m_cameraPos;
	renderPassCB.RenderTargetSize = XMFLOAT2(GetScreenWidth(), GetScreenHeight());
	renderPassCB.InvRenderTargetSize = XMFLOAT2(1.0f/GetScreenWidth(), 1.0f / GetScreenHeight());
	renderPassCB.NearZ = 1.0f;
	renderPassCB.FarZ = 1000.0f;
	renderPassCB.TotalTime = GetTotalTime();
	renderPassCB.DeltaTime = deltaTime;

	auto currentPassCB = m_currentFrameResource->PassConstantBuffer.get();
	currentPassCB->CopyData(0, renderPassCB);
}

void AstroGameInstance::CreatePasses(AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
	AstroDX::CommonMeshes::CreateCommonMeshes(*m_meshLibrary.get(), m_renderer->GetRendererContext());

	// --- Create all passes upfront ---

	// Base Geo
	auto baseGeoPass = std::make_shared<BasePassSceneGeometry>();
	baseGeoPass->Init(m_renderer.get(), shaderLibrary, *m_meshLibrary, NumFrameResources);
	m_gpuPasses.push_back(baseGeoPass);

	// Debug Draw
	auto debugDrawLinePass = std::make_shared<ComputePassVertexLineDebugDraw>();
	debugDrawLinePass->Init(m_renderer.get(), shaderLibrary);

	auto debugDrawRenderPass = std::make_shared<GraphicsPassDebugDraw>();
	debugDrawRenderPass->Init(m_renderer.get(), shaderLibrary, *m_meshLibrary.get());

	// Particle System
	auto particlesSimPass = std::make_shared<ComputePassParticles>();
	particlesSimPass->Init(m_renderer.get(), shaderLibrary);
	std::weak_ptr<ComputePassParticles> particleSimPassWeak = particlesSimPass;
	m_gpuPasses.push_back(particlesSimPass);

	auto particlesRenderPass = std::make_shared<GraphicsPassParticles>();
	particlesRenderPass->Init(particleSimPassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	m_gpuPasses.push_back(particlesRenderPass);

	// Physics Chain
	auto physicsChainSimPass = std::make_shared<ComputePassPhysicsChain>();
	physicsChainSimPass->Init(m_renderer.get(), shaderLibrary, debugDrawRenderPass->GetDebugObjectsBufferUAVIndex());
	std::weak_ptr<ComputePassPhysicsChain> physicsChainSimPassWeak = physicsChainSimPass;
	m_gpuPasses.push_back(physicsChainSimPass);

	auto physicsChainRenderPass = std::make_shared<GraphicsPassPhysicsChain>();
	physicsChainRenderPass->Init(physicsChainSimPassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	m_gpuPasses.push_back(physicsChainRenderPass);

	// Eulerian Fluid Sim 2D
	auto fluidSim2DComputePass = std::make_shared<ComputePassFluidSim2D>();
	fluidSim2DComputePass->Init(m_renderer.get(), shaderLibrary);
	std::weak_ptr<ComputePassFluidSim2D> fluidSim2DComputePassWeak = fluidSim2DComputePass;
	m_gpuPasses.push_back(fluidSim2DComputePass);

	auto fluidSim2DGraphicsPass = std::make_shared<GraphicsPassFluidSim2D>();
	fluidSim2DGraphicsPass->Init(fluidSim2DComputePassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	m_gpuPasses.push_back(fluidSim2DGraphicsPass);

	// PIC/FLIP 3D Fluid Sim
	auto fluidSim3DComputePass = std::make_shared<ComputePassPicFlip3D>();
	fluidSim3DComputePass->Init(m_renderer.get(), shaderLibrary, debugDrawLinePass);
	std::weak_ptr<ComputePassPicFlip3D> fluidSim3DComputePassWeak = fluidSim3DComputePass;
	m_gpuPasses.push_back(fluidSim3DComputePass);

	auto fluidSim3DGraphicsPass = std::make_shared<GraphicsPassPicFlip3D>();
	fluidSim3DGraphicsPass->Init(fluidSim3DComputePassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	m_gpuPasses.push_back(fluidSim3DGraphicsPass);

	// Debug draw passes render last to overlay on top
	m_gpuPasses.push_back(debugDrawLinePass);
	m_gpuPasses.push_back(debugDrawRenderPass);

	// Raymarch SDF Scene (depends on particle sim data)
	auto raymarchSDFScenePass = std::make_shared<ComputePassRaymarchScene>();
	raymarchSDFScenePass->Init(m_renderer.get(), shaderLibrary, particleSimPassWeak);
	const int32_t GBufferColorViewIndex = raymarchSDFScenePass->GetColorRTViewIndex();
	m_gpuPasses.push_back(raymarchSDFScenePass);

	auto copyGbufferToBackBufferPass = std::make_shared<GraphicsPassCopyGBufferToBackbuffer>();
	copyGbufferToBackBufferPass->Init(m_renderer.get(), shaderLibrary, GBufferColorViewIndex);
	m_gpuPasses.push_back(copyGbufferToBackBufferPass);

	// --- Register demos ---
	m_demoManager.RegisterDemo("BaseGeo",
		{ baseGeoPass.get() });

	m_demoManager.RegisterDemo("DebugDrawLine",
		{ debugDrawLinePass.get() });

	m_demoManager.RegisterDemo("DebugDrawRender",
		{ debugDrawRenderPass.get() });

	m_demoManager.RegisterDemo("Particles",
		{ particlesSimPass.get(), particlesRenderPass.get() });

	m_demoManager.RegisterDemo("PhysicsChain",
		{ physicsChainSimPass.get(), physicsChainRenderPass.get() },
		{ "DebugDrawRender" });

	m_demoManager.RegisterDemo("FluidSim2D",
		{ fluidSim2DComputePass.get(), fluidSim2DGraphicsPass.get() });

	m_demoManager.RegisterDemo("FluidSim3D",
		{ fluidSim3DComputePass.get(), fluidSim3DGraphicsPass.get() },
		{ "DebugDrawLine" });

	m_demoManager.RegisterDemo("RaymarchSDF",
		{ raymarchSDFScenePass.get(), copyGbufferToBackBufferPass.get() },
		{ "Particles" });

	// Load config (creates default file if none exists)
	const std::string configPath = GetWorkingDirectory() + "\\demos.cfg";
	m_demoManager.LoadConfig(configPath);
}

void AstroGameInstance::Update(float deltaTime, ivec2 cursorPos)
{
	Game::Update(deltaTime, cursorPos);

	m_frameIdx++;

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Frame Resource"); 
	UpdateFrameResource();
	PIXEndEvent();

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Const Data"); // See pch.h for info
	//PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

	// Convert Spherical to Cartesian coordinates.

	XMStoreFloat3(&m_cameraPos, m_cameraOriginPos);

	//Build View Matrix
	XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX cameraRotation = XMMatrixRotationRollPitchYaw(
		m_cameraPhi,
		m_cameraTheta,
		0.0f);
	m_lookDir = XMVector3TransformNormal(
		XMVectorSet(0.f, 0.f, 1.f, 0.0f),
		cameraRotation);
	XMVECTOR right = XMVector3Normalize( XMVector3Cross(worldUp, m_lookDir));
	XMVECTOR up = XMVector3Normalize( XMVector3Cross( m_lookDir, right));

	XMVECTOR LookAtPos = XMVectorAdd(m_cameraOriginPos, m_lookDir);
	XMMATRIX view = XMMatrixLookAtLH(m_cameraOriginPos, LookAtPos, up);
	XMStoreFloat4x4(&m_viewMat, view);
	PIXEndEvent();

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Constant Buffers Objects & Main Render pass");
	
	UpdateMainRenderPassConstantBuffer(deltaTime);

	const int32_t frameIdxModulo = m_frameIdx % NumFrameResources;

	const GPUPassUpdateData updateData
	{
		deltaTime,
		frameIdxModulo,
		cursorPos
	};

	for (auto& gpuPass : m_gpuPasses)
	{
		if (gpuPass->IsEnabled())
			gpuPass->Update(updateData);
	}

	PIXEndEvent();
}

void AstroGameInstance::Render(float deltaTime)
{
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Render");

	m_renderer->StartNewFrame(m_currentFrameResource);

	for (auto& pass : m_gpuPasses)
	{
		if (!pass->IsEnabled())
			continue;

		m_renderer->ProcessGPUPass(*pass.get(), *m_currentFrameResource, deltaTime);
		
		switch (pass->PassType())
		{
		case GPUPassType::Graphics:
			break;
		case GPUPassType::Compute:
			break;
		default:
			DX::astro_assert(false, "Pass type not supported");
		}
	}

	m_renderer->EndNewFrame([&](int newFenceValue) {
		m_currentFrameResource->Fence = newFenceValue;
		});

	PIXEndEvent();
}

void AstroGameInstance::OnSimReset()
{
	for (auto& pass : m_gpuPasses)
	{
		pass->OnSimReset();
	}
}

void AstroGameInstance::Shutdown()
{
	m_demoManager.SaveConfig();

	for (auto& pass : m_gpuPasses)
	{
		pass->Shutdown();
	}
	m_gpuPasses.clear();

	Game::Shutdown();
}