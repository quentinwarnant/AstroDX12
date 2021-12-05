#include "AstroGameInstance.h"

#include "Rendering/RenderData/VertexData.h"
#include "Rendering/RenderData/VertexDataFactory.h"

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
	for (auto& renderableDesc : m_renderablesDesc)
	{
		renderableDesc.ConstantBuffer = m_renderer->CreateConstantBuffer<RenderableObjectConstantData>(1);//TODO: make cbuffer defined by scene data we'd load, instead of fixed

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = renderableDesc.ConstantBuffer->Resource()->GetGPUVirtualAddress();

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
		cbViewDesc.BufferLocation = cbAddress;
		cbViewDesc.SizeInBytes = renderableDesc.ConstantBuffer->GetElementByteSize();

		// Finalise creation of constant buffer view
		m_renderer->CreateConstantBufferView(cbViewDesc);
	}
}

void AstroGameInstance::BuildRootSignature()
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		CD3DX12_ROOT_PARAMETER slotRootParams[1] = {};

		// Descriptor table of 1 CBV 
		CD3DX12_DESCRIPTOR_RANGE cbvTable;
		cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		slotRootParams[0].InitAsDescriptorTable(1, &cbvTable);

		// Root signature is an array of root parameters
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, slotRootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void AstroGameInstance::BuildShadersAndInputLayout()
{
	const auto rootPath = DX::GetWorkingDirectory();

	// TODO: associate this to a renderable instead of being global - although would be useful to also have a fallback shader available globally for failed compilations
	m_vertexShaderByteCode = AstroTools::Rendering::CompileShader(rootPath + L"\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	m_pixelShaderByteCode = AstroTools::Rendering::CompileShader(rootPath + L"\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	m_inputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
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

	
	std::unique_ptr<Mesh> boxMesh = std::make_unique<Mesh>();
	boxMesh->Name = "BoxGeometry";
	
	const auto vertsPODList = VertexDataFactory::Convert(verts);
	constexpr auto podDataSize = VertexDataFactory::GetPODTypeSize<VertexData_Short>();
	m_renderer->CreateMesh(boxMesh, vertsPODList.data(), (UINT)vertsPODList.size(), podDataSize, indices);

	m_renderablesDesc.emplace_back(std::move(boxMesh));
}

//TODO: move part of this to renderer class
void AstroGameInstance::BuildPipelineStateObject()
{
	m_renderer->CreateGraphicsPipelineState(
		m_pipelineStateObject, 
		m_renderablesDesc[0].RootSignature, /*TODO: TEMP until I figure out if root signatures are compatible across multiple objects*/
		m_inputLayout,
		m_vertexShaderByteCode, 
		m_pixelShaderByteCode);
}

void AstroGameInstance::CreateRenderables()
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		auto renderableObj = std::make_shared<RenderableStaticObject>(renderableDesc.Mesh, renderableDesc.RootSignature, renderableDesc.ConstantBuffer);
		AddRenderable(renderableObj);
	}
}

void AstroGameInstance::Update(float deltaTime)
{
	Game::Update(deltaTime);

	PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update"); // See pch.h for info
	//PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

	// Convert Spherical to Cartesian coordinates.
	constexpr float cameraRadius = 35.0f;
	float x = cameraRadius * sinf(m_cameraPhi) * cosf(m_cameraTheta);
	float z = cameraRadius * sinf(m_cameraPhi) * sinf(m_cameraTheta);
	float y = cameraRadius * cosf(m_cameraPhi);

	//Build View Matrix
	XMVECTOR pos = XMVectorSet(x,y,z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_viewMat, view);

	XMMATRIX world = XMLoadFloat4x4(&m_worldMat);
	XMMATRIX proj = XMLoadFloat4x4(&m_projMat);
	XMMATRIX worldViewProj = world * view * proj;

	//Update constant buffer with updated worldViewProj value
	RenderableObjectConstantData objectConstants;
	XMStoreFloat4x4(&objectConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	m_sceneRenderables[0]->SetConstantBufferData(&objectConstants);

	PIXEndEvent();
}

void AstroGameInstance::Render(float deltaTime)
{
	m_renderer->Render(deltaTime, m_sceneRenderables, m_pipelineStateObject);

}
