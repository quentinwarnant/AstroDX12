#include "AstroGameInstance.h"

void AstroGameInstance::BuildConstantBuffers()
{
	for (auto& renderableDesc : m_renderablesDesc)
	{
		renderableDesc.ConstantBuffer = m_renderer->CreateConstantBuffer<RenderableObjectConstantData>(1);//TODO: make cbuffer defined by scene data we'd load, instead of fixed

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = renderableDesc.ConstantBuffer->Resource()->GetGPUVirtualAddress();

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc;
		cbViewDesc.BufferLocation = cbAddress;
		cbViewDesc.SizeInBytes = renderableDesc.ConstantBuffer->GetElementByteSize();

		//Finalise creation of constant buffer view
		m_renderer->CreateConstantBufferView(cbViewDesc);
	}
}

void AstroGameInstance::BuildRootSignature()
{
}

void AstroGameInstance::BuildShadersAndInputLayout()
{

}

void AstroGameInstance::BuildSceneGeometry()
{

}

void AstroGameInstance::BuildPipelineStateObject()
{

}

void AstroGameInstance::LoadSceneData()
{
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_projMat, proj);

	// Create Scene objects
	m_renderablesDesc.clear();
	auto mesh = std::make_unique<Mesh>();
	// TODO : init mesh with vert & indices data

	m_renderablesDesc.emplace_back( std::move(mesh), m_rootSignature );
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

	//Build View Matrix
	XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -100.0f, 1.0f);
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
