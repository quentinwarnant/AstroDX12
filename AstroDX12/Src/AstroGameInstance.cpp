#include <AstroGameInstance.h>

#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/RendererProcessedObjectType.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/RenderData/VertexDataFactory.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/VertexDataInputLayoutLibrary.h>
#include <Rendering/Compute/ComputableObject.h>

#include <GameContent/GPUPasses/BasePassSceneGeometry.h>
#include <GameContent/GPUPasses/Compute/ComputePassAddBufferValues.h>
#include <GameContent/GPUPasses/Compute/ComputePassParticles.h>

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
	, m_renderablesDesc()
	, m_computableDescs()
	, m_cameraPos(0,0,0)
	, m_frameResources()
	, m_currentFrameResource(nullptr)
	, m_currentFrameResourceIndex(0)
	, m_meshLibrary(std::make_unique<MeshLibrary>())
{
}

void AstroGameInstance::LoadSceneData()
{
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_projMat, proj);

	// Create Scene objects
	m_renderablesDesc.clear();

	BuildSceneGeometry();
	BuildComputeData();
}

void AstroGameInstance::Create_const_uav_srv_BufferDescriptorHeaps()
{
	m_renderer->Create_const_uav_srv_BufferDescriptorHeaps();

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
		m_frameResources[frameIdx]->ComputableObjectPerObjDataCBVgpuAddress.push_back(cbAddress);
	}
}

void AstroGameInstance::CreateComputableObjectsStructuredBufferViews()
{
	size_t computableObjCount = m_computableDescs.size();

	// Create CBV descriptors for each "renderable" & "computable" object in each frame resource
	for (int16_t frameIdx = 0; frameIdx < NumFrameResources; ++frameIdx)
	{
		// Computable objects each have their own buffer, instead of one shared buffer (testing different architecture / ease of maintainability until better memory management is implemented)
		for (size_t computableObjIdx = 0; computableObjIdx < computableObjCount; ++computableObjIdx)
		{

			m_renderer->CreateStructuredBufferAndViews(
				m_frameResources[frameIdx]->ComputableObjectStructuredBufferPerObj[computableObjIdx].get(),
				true,
				false
			);

			m_renderer->CreateStructuredBufferAndViews(
				m_frameResources[frameIdx]->ComputableObjectStructuredBufferPerObj[computableObjIdx + 1].get(),
				true,
				false
			);

			m_renderer->CreateStructuredBufferAndViews(
				m_frameResources[frameIdx]->ComputableObjectStructuredBufferPerObj[computableObjIdx + 2].get(),
				false,
				true
			);
		}
	}
}

void AstroGameInstance::BuildRootSignature()
{
	for (auto& renderableDesc : m_renderablesDesc) // TODO: move to BasePassSceneGeometry Pass 
	{
		std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

		// Descriptor table of 3 CBV:
		// one per pass,
		// one per object (transform) (object transform - to be depricated in favor of next one),
		// one for the object's bindless resource indices 
	
		const D3D12_ROOT_PARAMETER1 rootParamCBVPerPass
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
			.Descriptor
			{
				.ShaderRegister = 0,
				.RegisterSpace = 0,
				.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC
			}
		};
		slotRootParams.push_back(rootParamCBVPerPass);

		const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants
			{
				.ShaderRegister = 1,
				.RegisterSpace = 0,
				.Num32BitValues = 3
			}
		};
		slotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

		//if (renderableDesc.GetSupportsTextures())
		//{
		//	CD3DX12_DESCRIPTOR_RANGE textureDescriptorRangeTable0;
		//	textureDescriptorRangeTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BINDLESS_TEXTURE2D_TABLE_SIZE, 0, TEXTURE2D_DESCRIPTOR_SPACE);
		//	slotRootParams.push_back(CD3DX12_ROOT_PARAMETER());
		//	slotRootParams[slotRootParams.size() - 1].InitAsDescriptorTable(1, & textureDescriptorRangeTable0, D3D12_SHADER_VISIBILITY_ALL);
		//}

		// Root signature is an array of root parameters
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			(UINT)slotRootParams.size(),
			slotRootParams.data(),
			0,
			nullptr,
			  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED 
		);

		// Create the root signature
		ComPtr<ID3DBlob> serializedRootSignature = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			serializedRootSignature.GetAddressOf(),
			errorBlob.GetAddressOf());
		
		if (errorBlob)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ComPtr<ID3D12RootSignature> rootSignature = nullptr;
		m_renderer->CreateRootSignature(serializedRootSignature, rootSignature);

		renderableDesc.RootSignature = rootSignature;
	}

	for (auto& computableDesc : m_computableDescs)
	{
		CD3DX12_ROOT_PARAMETER slotRootParams[3] = {};
		// 2 SRV Structured buffers & 1 UAV Structured buffer

		slotRootParams[0].InitAsShaderResourceView(0);
		slotRootParams[1].InitAsShaderResourceView(1);
		slotRootParams[2].InitAsUnorderedAccessView(0);

		// Root signature is an array of root parameters
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			3,
			slotRootParams,
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Create the root signature
		ComPtr<ID3DBlob> serializedRootSignature = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
		if (errorBlob)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ComPtr<ID3D12RootSignature> rootSignature = nullptr;
		m_renderer->CreateRootSignature(serializedRootSignature, rootSignature);

		computableDesc.RootSignature = rootSignature;
	}
}

void AstroGameInstance::BuildShaders(AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		renderableDesc.VS = shaderLibrary.GetCompiledShader(renderableDesc.VertexShaderPath, L"VS", {}, L"vs_6_6");
		renderableDesc.PS = shaderLibrary.GetCompiledShader(renderableDesc.PixelShaderPath, L"PS", {}, L"ps_6_6");
	}

	for (auto& computableDesc : m_computableDescs)
	{
		computableDesc.CS = shaderLibrary.GetCompiledShader(computableDesc.ComputeShaderPath, L"CSMain", {}, L"cs_6_6");
	}
}

void AstroGameInstance::BuildSceneGeometry()
{
	auto rendererContext = m_renderer->GetRendererContext();

	std::vector<VertexData_Short> verts;
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, -1.5f, -1.5f), DirectX::XMFLOAT4(Colors::White)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, +1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Black)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, +1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Red)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, -1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Green)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, -1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Blue)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, +1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Yellow)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, +1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Cyan)));
	verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, -1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Magenta)));

	const std::vector<std::uint32_t> indices =
	{
		// Front face
		0, 1, 2,
		0, 2, 3,

		// Back face
		4, 6, 5,
		4, 7, 6,

		// Left face
		4, 5, 1,
		4, 1, 0,

		// Right face
		3, 2, 6, 
		3, 6, 7,

		// Top face
		1, 5, 6,
		1, 6, 2,

		// Bottom face
		4, 0, 3,
		4, 3, 7
	};

	
	const auto vertsPODList = VertexDataFactory::Convert(verts);
	auto boxMesh = m_meshLibrary->AddMesh(rendererContext, std::string("BoxGeometry"), vertsPODList, indices );

	const auto rootPath = s2ws(DX::GetWorkingDirectory());
	const auto basicShaderPath = rootPath + std::wstring(L"\\Shaders\\basic.hlsl");
	const auto vertexColorShaderPath = rootPath + std::wstring(L"\\Shaders\\color.hlsl");
	const auto simpleNormalUVAndLightingShaderPath = rootPath + std::wstring(L"\\Shaders\\simpleNormalUVLighting.hlsl");
	const auto texturedShaderPath = rootPath + std::wstring(L"\\Shaders\\Textured.hlsl");

	// Box 1
	auto transformBox1 = XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	m_renderablesDesc.emplace_back(
		boxMesh,
		vertexColorShaderPath, 
		vertexColorShaderPath, 
		AstroTools::Rendering::InputLayout::IL_Pos_Color,
		transformBox1,
		false);

	// Box 2
	auto transformBox2 = XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 10.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	m_renderablesDesc.emplace_back(
		boxMesh,
		vertexColorShaderPath,
		vertexColorShaderPath,
		AstroTools::Rendering::InputLayout::IL_Pos_Color,
		transformBox2,
		false);

	// Box 3
	auto transformBox3 = XMFLOAT4X4(
		4.0f, 0.0f, 0.0f, 20.0f,
		0.0f, 4.0f, 0.0f, 20.0f,
		0.0f, 0.0f, 4.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	m_renderablesDesc.emplace_back(
		boxMesh,
		vertexColorShaderPath,
		vertexColorShaderPath,
		AstroTools::Rendering::InputLayout::IL_Pos_Color,
		transformBox3,
		false);


	// .Obj load
	const auto SceneData = LoadSceneGeometry();
	for (const auto& SceneMeshObj : SceneData.SceneMeshObjects_VD_PosNormUV)
	{
		std::weak_ptr<IMesh> mesh;
		// Either find an existing mesh with the unique name or add a new one to the library
		if (!m_meshLibrary->GetMesh(SceneMeshObj.meshName, mesh) )
		{
			const auto newMeshvertsPODList = VertexDataFactory::Convert(SceneMeshObj.verts);
			mesh = m_meshLibrary->AddMesh(
				rendererContext,
				SceneMeshObj.meshName,
				newMeshvertsPODList,
				SceneMeshObj.indices
			);
		}

		m_renderablesDesc.emplace_back(
			mesh,
			simpleNormalUVAndLightingShaderPath,
			simpleNormalUVAndLightingShaderPath,
			AstroTools::Rendering::InputLayout::IL_Pos_Normal_UV,
			SceneMeshObj.transform,
			false);
	}
}

void AstroGameInstance::BuildFrameResources()
{
	m_renderer->BuildFrameResources(m_frameResources, NumFrameResources, (int)m_renderablesDesc.size(), (int)m_computableDescs.size());
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

void AstroGameInstance::BuildComputeData()
{
	const auto rootPath = s2ws(DX::GetWorkingDirectory());
	const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\computeTest1.hlsl");

	m_computableDescs.emplace_back(
		computeShaderPath,
		AstroTools::Rendering::InputLayout::IL_CS_Test1
	);
}

SceneData AstroGameInstance::LoadSceneGeometry()
{
	return SceneLoader::LoadScene(1);
}

void AstroGameInstance::BuildPipelineStateObject()
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		m_renderer->CreateGraphicsPipelineState(
			renderableDesc.PipelineStateObject,
			renderableDesc.RootSignature, 
			renderableDesc.InputLayout,
			renderableDesc.VS,
			renderableDesc.PS);
	}

	for (auto& computableDesc : m_computableDescs)
	{
		m_renderer->CreateComputePipelineState(
			computableDesc.PipelineStateObject,
			computableDesc.RootSignature,
			computableDesc.InputLayout,
			computableDesc.CS);
	}
}

void AstroGameInstance::CreatePasses(AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
	// Base Geo PASS
	auto BaseGeoPass = std::make_shared< BasePassSceneGeometry>();
	BaseGeoPass->Init(m_renderer.get(), m_renderablesDesc, NumFrameResources);
	m_gpuPasses.push_back(std::move(BaseGeoPass));

	auto AddTwoBuffersTogetherPass = std::make_shared< ComputePassAddBufferValues >();
	AddTwoBuffersTogetherPass->Init(m_computableDescs);
	m_gpuPasses.push_back(std::move(AddTwoBuffersTogetherPass));

	auto ParticlesSimPass = std::make_shared< ComputePassParticles >();	
	ParticlesSimPass->Init(m_renderer.get(), shaderLibrary);
	std::weak_ptr< ComputePassParticles > ParticleSimPassWeak = ParticlesSimPass;
	m_gpuPasses.push_back(std::move(ParticlesSimPass));

	auto ParticlesRenderPass = std::make_shared< GraphicsPassParticles >();
	ParticlesRenderPass->Init(ParticleSimPassWeak, m_renderer.get(), shaderLibrary, *m_meshLibrary.get());
	m_gpuPasses.push_back(std::move(ParticlesRenderPass));

	// TODO: call Shutdown on all gpu passes
}


void AstroGameInstance::Update(float deltaTime)
{
	Game::Update(deltaTime);

	m_frameIdx++;

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Frame Resource"); 
	UpdateFrameResource();
	PIXEndEvent();

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Const Data"); // See pch.h for info
	//PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

	// Convert Spherical to Cartesian coordinates.
	constexpr float cameraRadius = 35.0f;

	float LookAtDirX = cosf(m_cameraTheta);
	float LookAtDirY = sinf(m_cameraPhi);
	float LookAtDirZ = sinf(m_cameraTheta) * cosf(m_cameraPhi);
	m_lookDir = XMVector3Normalize(XMVectorSet(LookAtDirX, LookAtDirY, LookAtDirZ, 0.f));
	float posX = XMVectorGetX(m_cameraOriginPos);
	float posY = XMVectorGetY(m_cameraOriginPos);
	float posZ = XMVectorGetZ(m_cameraOriginPos);
	float LookAtX = posX + LookAtDirX * cameraRadius;
	float LookAtY = posY + LookAtDirY * cameraRadius;
	float LookAtZ = posZ + LookAtDirZ * cameraRadius;

	XMVECTOR pos = XMVectorSet(posX, posY, posZ, 1.0f);
	XMStoreFloat3(&m_cameraPos, pos);

	//Build View Matrix
	XMVECTOR target = XMVectorSet(LookAtX, LookAtY, LookAtZ, 0.f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_viewMat, view);
	PIXEndEvent();

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Constant Buffers Objects & Main Render pass");
	
	UpdateMainRenderPassConstantBuffer(deltaTime);

	const int32_t frameIdxModulo = m_frameIdx % NumFrameResources;

	for (auto& gpuPass : m_gpuPasses)
	{
		gpuPass.get()->Update(frameIdxModulo, nullptr);
	}

	PIXEndEvent();
}

void AstroGameInstance::Render(float /*deltaTime*/)
{
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Render");

	m_renderer->StartNewFrame(m_currentFrameResource);


	for (auto& pass : m_gpuPasses)
	{
		m_renderer->ProcessGPUPass(*pass.get(), *m_currentFrameResource);
		
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

void AstroGameInstance::Shutdown()
{
	for (auto& pass : m_gpuPasses)
	{
		pass->Shutdown();
	}
	m_gpuPasses.clear();
}