#include "GraphicsPassDebugDraw.h"
#include "Rendering/RenderData/RenderConstants.h"
#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/Common/RendererContext.h>
#include <Rendering/Common/FrameResource.h>

namespace Privates
{
	static const uint32_t MaxDebugObjects = 1000;
}

void GraphicsPassDebugDraw::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary)
{
    auto BufferDataVector = std::vector<GraphicsPassDebugDraw::DebugObjectData>(Privates::MaxDebugObjects);
    for (int32_t idx = 0; idx < Privates::MaxDebugObjects; ++idx)
    {
        BufferDataVector[idx].Transform = AstroTools::Maths::Identity4x4();
		BufferDataVector[idx].Color = DirectX::XMFLOAT3(1.f, 0.f, 0.f);
    }

    m_debugObjectsBuffer = std::make_unique<StructuredBuffer<GraphicsPassDebugDraw::DebugObjectData>>(BufferDataVector);
    renderer->CreateStructuredBufferAndViews(m_debugObjectsBuffer.get(), true, true);

	CreateRootSignature(renderer);
	CreatePipelineState(renderer, shaderLibrary);
	CreateMesh(renderer->GetRendererContext(), meshLibrary);
}

void GraphicsPassDebugDraw::CreateRootSignature(IRenderer* renderer)
{

	std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

	// Descriptor table of 2 entries CBV:
	// one per pass, (CBV)
	// one with bindless resource indices (constants)

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
			.Num32BitValues = 2 // index to the structured buffer containing the debug objects data
		}
	};
	slotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

	//TODO support draw indirect, by writing dispatch data to buffer and scheduling draw passes. Other passes should write to this buffer

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

	renderer->CreateRootSignature(serializedRootSignature, m_rootSignature);

}

void GraphicsPassDebugDraw::CreateMesh(RendererContext& rendererContext, MeshLibrary& meshLibrary)
{
	// sphere mesh
	std::vector<VertexData_Position_POD> vertices;
	std::vector<uint32_t> indices;

	const int32_t latitudeCount = 10;
	const int32_t longitudeCount = 10;
	const float radius = 0.5f;

	for (int32_t lat = 0; lat <= latitudeCount; ++lat)
	{
		float theta = lat * XM_PI / latitudeCount;
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);

		for (int32_t lon = 0; lon <= longitudeCount; ++lon)
		{
			float phi = lon * 2.0f * XM_PI / longitudeCount;
			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			XMFLOAT3 position = { radius * sinTheta * cosPhi, radius * cosTheta, radius * sinTheta * sinPhi };
			vertices.push_back(position);
		}
	}

	for (int32_t lat = 0; lat < latitudeCount; ++lat)
	{
		for (int32_t lon = 0; lon < longitudeCount; ++lon)
		{
			indices.push_back(lat * (longitudeCount + 1) + lon);
			indices.push_back((lat + 1) * (longitudeCount + 1) + lon);
			indices.push_back(lat * (longitudeCount + 1) + (lon + 1));

			indices.push_back((lat + 1) * (longitudeCount + 1) + lon);
			indices.push_back((lat + 1) * (longitudeCount + 1) + (lon + 1));
			indices.push_back(lat * (longitudeCount + 1) + (lon + 1));
		}
	}

	m_debugMesh = meshLibrary.AddMesh(
		rendererContext, 
		std::string("Sphere"),
		vertices,
		indices);
}

void GraphicsPassDebugDraw::CreatePipelineState(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
	const std::wstring shaderPath = L"Shaders/DebugDraw.hlsl";

	auto VS = shaderLibrary.GetCompiledShader(shaderPath, L"VS", {}, L"vs_6_6");
	auto PS = shaderLibrary.GetCompiledShader(shaderPath, L"PS", {}, L"ps_6_6");

	renderer->CreateGraphicsPipelineState(
		m_pso,
		m_rootSignature,
		nullptr,
		VS,
		PS, 
		/*wireframe*/ true);
}

void GraphicsPassDebugDraw::Update(int32_t /*frameIdxModulo*/, void* /*Data*/)
{
}

void GraphicsPassDebugDraw::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float /*deltaTime*/, const FrameResource& frameResources) const
{
	PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "GraphicsPassDebugDraw");

	const auto indexBuffer = m_debugMesh.lock()->IndexBufferView();
	cmdList->IASetIndexBuffer(&indexBuffer);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	cmdList->SetPipelineState(m_pso.Get());

	const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
	cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);

	const uint32_t GraphicsBindlessResourceIndicesRootSigParamIndex = 1;
	const std::vector<int32_t> GraphicsBindlessResourceIndices = {
		m_debugMesh.lock()->GetVertexBufferSRV(),
		m_debugObjectsBuffer->GetSRVIndex()
	};
	cmdList->SetGraphicsRoot32BitConstants(
		(UINT)GraphicsBindlessResourceIndicesRootSigParamIndex,
		(UINT)GraphicsBindlessResourceIndices.size(), GraphicsBindlessResourceIndices.data(), 0);

	cmdList->DrawIndexedInstanced((UINT)m_debugMesh.lock()->GetVertexIndicesCount(), Privates::MaxDebugObjects, 0, 0, 0);

	//cmdList->ExecuteIndirect()

}

void GraphicsPassDebugDraw::Shutdown()
{
	m_debugMesh.reset();
	m_rootSignature = nullptr;
	m_pso = nullptr;
}
