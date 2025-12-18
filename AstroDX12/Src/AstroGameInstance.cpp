#include <AstroGameInstance.h>

#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/RenderData/VertexDataFactory.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/VertexDataInputLayoutLibrary.h>
#include <Rendering/Compute/ComputableObject.h>
#include <Rendering/Common/VectorTypes.h>


#include <GameContent/GPUPasses/BasePassSceneGeometry.h>
#include <GameContent/GPUPasses/Compute/ComputePassParticles.h>
#include <GameContent/GPUPasses/Compute/ComputePassRaymarchScene.h>
#include <GameContent/GPUPasses/GraphicsPassCopyGBufferToBackbuffer.h>
#include <GameContent/GPUPasses/Compute/ComputePassPhysicsChain.h>
#include <GameContent/GPUPasses/Debugging/GraphicsPassDebugDraw.h>
#include <GameContent/GPUPasses/Compute/ComputePassFluidSim2D.h>

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
	m_frameResources.clear();
	m_meshLibrary.reset();
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
		const auto cbvGpuHandle = m_renderer->CreateConstantBufferView(cbAddress, passCBByteSize);
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
	 //Base Geo PASS
	//auto baseGeoPass = std::make_shared< BasePassSceneGeometry>();
	//baseGeoPass->Init(m_renderer.get(),shaderLibrary, *m_meshLibrary, NumFrameResources);
	//m_gpuPasses.push_back(std::move(baseGeoPass));

	//auto debugDrawRenderPass = std::make_shared< GraphicsPassDebugDraw >();
	//debugDrawRenderPass->Init(m_renderer.get(), shaderLibrary, *m_meshLibrary.get());

	// //Particle System Passes
	//auto particlesSimPass = std::make_shared< ComputePassParticles >();	
	//particlesSimPass->Init(m_renderer.get(), shaderLibrary);
	//std::weak_ptr< ComputePassParticles > ParticleSimPassWeak = particlesSimPass;
	//m_gpuPasses.push_back(std::move(particlesSimPass));

	//auto particlesRenderPass = std::make_shared< GraphicsPassParticles >();
	//particlesRenderPass->Init(ParticleSimPassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	//m_gpuPasses.push_back(std::move(particlesRenderPass));

	//// Physics chain passes
	//auto physicsChainSimPass = std::make_shared< ComputePassPhysicsChain >();
	//physicsChainSimPass->Init(m_renderer.get(), shaderLibrary, debugDrawRenderPass->GetDebugObjectsBufferUAVIndex());
	//std::weak_ptr< ComputePassPhysicsChain > physicsChainSimPassWeak = physicsChainSimPass;
	//m_gpuPasses.push_back(std::move(physicsChainSimPass));

	//auto physicsChainRenderPass = std::make_shared< GraphicsPassPhysicsChain >();
	//physicsChainRenderPass->Init(physicsChainSimPassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	//m_gpuPasses.push_back(std::move(physicsChainRenderPass));

	auto fluidSim2DComputePass = std::make_shared< ComputePassFluidSim2D >();
	fluidSim2DComputePass->Init(m_renderer.get(), shaderLibrary);

	auto fluidSim2DGraphicsPass = std::make_shared< GraphicsPassFluidSim2D >();
	fluidSim2DGraphicsPass->Init(fluidSim2DComputePass,m_renderer.get(), shaderLibrary, *m_meshLibrary.get());

	m_gpuPasses.push_back(std::move(fluidSim2DComputePass));
	m_gpuPasses.push_back(std::move(fluidSim2DGraphicsPass));


	 //Debug draw is the last pass to render debug objects on top of everything else
	//m_gpuPasses.push_back(std::move(debugDrawRenderPass));
	
	//auto raymarchSDFScenePass = std::make_shared<ComputePassRaymarchScene>();
	//raymarchSDFScenePass->Init(m_renderer.get(), shaderLibrary, ParticleSimPassWeak);
	//const int32_t GBufferResourceViewIndex = raymarchSDFScenePass->GetGBufferRTViewIndex();
	//m_gpuPasses.push_back(std::move(raymarchSDFScenePass));

	//auto copyGbufferToBackBufferPass = std::make_shared<GraphicsPassCopyGBufferToBackbuffer>();
	//copyGbufferToBackBufferPass->Init(m_renderer.get(), shaderLibrary, GBufferResourceViewIndex);
	//m_gpuPasses.push_back(std::move(copyGbufferToBackBufferPass));
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
		gpuPass.get()->Update(updateData);
	}

	PIXEndEvent();
}

void AstroGameInstance::Render(float deltaTime)
{
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Render");

	m_renderer->StartNewFrame(m_currentFrameResource);

	for (auto& pass : m_gpuPasses)
	{
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
	for (auto& pass : m_gpuPasses)
	{
		pass->Shutdown();
	}
	m_gpuPasses.clear();
}