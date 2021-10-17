#include "AstroGameInstance.h"

void AstroGameInstance::BuildConstantBuffers()
{

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

void AstroGameInstance::Setup()
{
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_projMat, proj);

	// Create Scene objects
	auto testObjMesh = Mesh();
	//Mesh& mesh, ComPtr<ID3D12RootSignature>& rootSignature
	auto testObj = std::make_shared<RenderableStaticObject>(testObjMesh, m_rootSignature);
	AddRenderable(testObj);
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
	m_renderableObjectConstantBuffer->CopyData(0, objectConstants);


	PIXEndEvent();
}

void AstroGameInstance::Render(float deltaTime)
{
	m_renderer->Render(deltaTime, m_sceneRenderables, m_pipelineStateObject, m_constantBufferViewHeap);

}
