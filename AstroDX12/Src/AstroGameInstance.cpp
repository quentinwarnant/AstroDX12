#include <AstroGameInstance.h>

#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/RenderData/VertexDataFactory.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/VertexDataInputLayoutLibrary.h>

namespace
{
	namespace
	{
		constexpr size_t NumFrameResources = 3;
	}
}

AstroGameInstance::AstroGameInstance()
	: Game()
	, m_renderablesDesc()
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
}

void AstroGameInstance::BuildConstantBuffers()
{
	size_t renderableObjCount = m_renderablesDesc.size();
	m_renderer->CreateConstantBufferDescriptorHeaps(NumFrameResources, renderableObjCount);

	// Create CBV descriptors for each object in each frame resource
	for (int16_t frameIdx = 0; frameIdx < NumFrameResources; ++frameIdx)
	{
		UINT objCBByteSize = m_frameResources[frameIdx]->ObjectConstantBuffer->GetElementByteSize();
		auto objectCB = m_frameResources[frameIdx]->ObjectConstantBuffer->Resource();
		for (size_t renderableObjIdx = 0; renderableObjIdx < renderableObjCount; ++renderableObjIdx)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
			// offset address to the i-th object constant buffer in the current frame resource buffer
			cbAddress += (((UINT)renderableObjIdx) * objCBByteSize);

			// Offset handle in the CBV descriptor heap
			size_t heapIdx = (frameIdx * renderableObjCount) + renderableObjIdx;

			// Finalise creation of constant buffer view
			m_renderer->CreateConstantBufferView(cbAddress, objCBByteSize, heapIdx );
		}
	}

	// Create CBV descriptors shared for the whole render pass, in each frame resource
	for (int16_t frameIdx = 0; frameIdx < NumFrameResources; ++frameIdx)
	{
		UINT passCBByteSize = m_frameResources[frameIdx]->PassConstantBuffer->GetElementByteSize();
		auto passCB = m_frameResources[frameIdx]->PassConstantBuffer->Resource();

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Offset handle in CBV Descriptor heap
		size_t heapIdx = (renderableObjCount * NumFrameResources) + frameIdx;

		// Finalise creation of constant buffer view
		m_renderer->CreateConstantBufferView(cbAddress, passCBByteSize, heapIdx);
	}

	m_renderer->SetPassCBVOffset(renderableObjCount * NumFrameResources);

}

void AstroGameInstance::BuildRootSignature()
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		CD3DX12_ROOT_PARAMETER slotRootParams[2] = {};

		// Descriptor table of 2 CBV - one per pass, one per object
		CD3DX12_DESCRIPTOR_RANGE cbvDescriptorRange0;
		cbvDescriptorRange0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		slotRootParams[0].InitAsDescriptorTable(1, &cbvDescriptorRange0);

		CD3DX12_DESCRIPTOR_RANGE cbvDescriptorRangeTable1;
		cbvDescriptorRangeTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
		slotRootParams[1].InitAsDescriptorTable(1, &cbvDescriptorRangeTable1);

		// Root signature is an array of root parameters
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, slotRootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

		renderableDesc.RootSignature = rootSignature;
	}
}

void AstroGameInstance::BuildShaders(AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		renderableDesc.VS = shaderLibrary.GetCompiledShader(renderableDesc.VertexShaderPath, "VS", "vs_5_0");
		renderableDesc.PS = shaderLibrary.GetCompiledShader(renderableDesc.PixelShaderPath, "PS", "ps_5_0");
	}
}

void AstroGameInstance::BuildSceneGeometry()
{
	std::vector< std::unique_ptr<VertexData_Short> > verts;
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(-1.5f, -1.5f, -1.5f), DirectX::XMFLOAT4(Colors::White)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(-1.5f, +1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Black)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(+1.5f, +1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Red)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(+1.5f, -1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Green)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(-1.5f, -1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Blue)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(-1.5f, +1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Yellow)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(+1.5f, +1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Cyan)));
	verts.emplace_back(std::make_unique<VertexData_Short>(DirectX::XMFLOAT3(+1.5f, -1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Magenta)));

	const std::vector<std::uint16_t> indices =
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

	auto boxMesh = m_meshLibrary->AddMesh("BoxGeometry");
	
	const auto vertsPODList = VertexDataFactory::Convert(verts);
	constexpr auto podDataSize = VertexDataFactory::GetPODTypeSize<VertexData_Short>();
	m_renderer->CreateMesh(boxMesh, vertsPODList.data(), (UINT)vertsPODList.size(), podDataSize, indices);

	const auto rootPath = DX::GetWorkingDirectory();
	const auto defaultShaderPath = rootPath + std::string("\\Shaders\\color.hlsl");

	// Box 1
	auto transformBox1 = XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	m_renderablesDesc.emplace_back(
		boxMesh,
		defaultShaderPath, 
		defaultShaderPath, 
		AstroTools::Rendering::InputLayout::IL_Pos_Color,
		transformBox1);

	// Box 2
	auto transformBox2 = XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		10.0f, 0.0f, 0.0f, 1.0f);
	m_renderablesDesc.emplace_back(
		boxMesh,
		defaultShaderPath,
		defaultShaderPath,
		AstroTools::Rendering::InputLayout::IL_Pos_Color,
		transformBox2);

	// Box 3
	auto transformBox3 = XMFLOAT4X4(
		2.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 2.0f, 0.0f,
		0.0f, 10.0f, 10.0f, 1.0f);
	m_renderablesDesc.emplace_back(
		boxMesh,
		defaultShaderPath,
		defaultShaderPath,
		AstroTools::Rendering::InputLayout::IL_Pos_Color,
		transformBox3);
}

void AstroGameInstance::BuildFrameResources()
{
	m_renderer->BuildFrameResources(m_frameResources, NumFrameResources, (int)m_renderablesDesc.size());
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

void AstroGameInstance::UpdateRenderablesConstantBuffers()
{
	// re-using the same constant buffer to set all the renderables objects - per object constant data.
	auto currentFrameObjectCB = m_currentFrameResource->ObjectConstantBuffer.get();
	for (auto& renderableGroupPair : m_renderableGroups)
	{
		renderableGroupPair.second->ForEach([&](std::shared_ptr<IRenderable> renderable)
		{
			if (renderable->IsDirty())
			{
				XMMATRIX worldTransform = XMLoadFloat4x4(&renderable->GetWorldTransform());
				RenderableObjectConstantData objectConstants;
				XMStoreFloat4x4(&objectConstants.WorldTransform, XMMatrixTranspose(worldTransform));
				currentFrameObjectCB->CopyData(renderable->GetConstantBufferIndex(), objectConstants);

				renderable->ReduceDirtyFrameCount();
			}
		});
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
}

void AstroGameInstance::CreateRenderables()
{
	size_t index = 0;
	for (auto& renderableDesc : m_renderablesDesc)
	{
		auto renderableObj = std::make_shared<RenderableStaticObject>(
			renderableDesc.Mesh,
			renderableDesc.RootSignature, 
			renderableDesc.PipelineStateObject,
			renderableDesc.InitialTransform,
			index++
			);
		renderableObj->MarkDirty(NumFrameResources);
		AddRenderable(std::move(renderableObj));
	}
}

void AstroGameInstance::Update(float deltaTime)
{
	Game::Update(deltaTime);

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Frame Resource"); 
	UpdateFrameResource();
	PIXEndEvent();

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Const Data"); // See pch.h for info
	//PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

	// Convert Spherical to Cartesian coordinates.
	constexpr float cameraRadius = 35.0f;
	float x = cameraRadius * sinf(m_cameraPhi) * cosf(m_cameraTheta);
	float z = cameraRadius * sinf(m_cameraPhi) * sinf(m_cameraTheta);
	float y = cameraRadius * cosf(m_cameraPhi);

	XMVECTOR pos = XMVectorSet(x,y,z, 1.0f);
	XMStoreFloat3(&m_cameraPos, pos);

	//Build View Matrix
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_viewMat, view);
	PIXEndEvent();

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update Constant Buffers Objects & Main Render pass");
	UpdateRenderablesConstantBuffers();
	UpdateMainRenderPassConstantBuffer(deltaTime);
	PIXEndEvent();
}

void AstroGameInstance::Render(float deltaTime)
{
	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Render");
	m_renderer->Render(deltaTime, m_renderableGroups, m_currentFrameResource, 
		[&](int newFenceValue) {
			m_currentFrameResource->Fence = newFenceValue;
		});
	PIXEndEvent();

}
