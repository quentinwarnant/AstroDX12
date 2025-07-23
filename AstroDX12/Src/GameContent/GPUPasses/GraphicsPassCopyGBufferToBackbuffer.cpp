#include "GraphicsPassCopyGBufferToBackbuffer.h"

#include <Common.h>
#include <Rendering\IRenderer.h>
#include <Rendering\Common\RenderingUtils.h>
#include <Rendering\Common\RendererContext.h>
#include <GameContent/GPUPasses/RaymarchScene.h>

namespace PassPrivates
{
	static const std::vector<uint32_t> VertexIndices = {
		0, 1, 2, // Triangle 1
		1, 3, 2  // Triangle 2
	};

	static void CreateRootSignatureAndPSO(AstroTools::Rendering::ShaderLibrary& shaderLibrary, IRenderer& renderer, ComPtr<ID3D12RootSignature>& outRootSignature, ComPtr<ID3D12PipelineState>& outPipelineStateObject)
	{
        const auto rootPath = s2ws(DX::GetWorkingDirectory());

        const auto shaderPath = rootPath + std::wstring(L"\\Shaders\\CopyGBufferToBackBuffer.hlsl");

        const std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout
        {
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        auto vs = shaderLibrary.GetCompiledShader(shaderPath, L"VS", {}, L"vs_6_6");
        auto ps = shaderLibrary.GetCompiledShader(shaderPath, L"PS", {}, L"ps_6_6");

        std::vector<D3D12_ROOT_PARAMETER1> rootSigSlotRootParams;

        const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
				.Num32BitValues = 3 // Number of bindless resource indices
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
        rootSigSlotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

        // Root signature is an array of root parameters
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
            (UINT)rootSigSlotRootParams.size(),
            rootSigSlotRootParams.data(),
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
        renderer.CreateRootSignature(serializedRootSignature, outRootSignature);

        renderer.CreateGraphicsPipelineState(
            outPipelineStateObject,
            outRootSignature,
            &inputLayout,
            vs,
            ps);
	}
}

void GraphicsPassCopyGBufferToBackbuffer::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, int32_t GBufferRTViewIndex)
{
    RendererContext rendererContext = renderer->GetRendererContext();

	const UINT64 IndexBufferByteSize = PassPrivates::VertexIndices.size() * sizeof(uint32_t);

	m_indexBufferGPU = AstroTools::Rendering::CreateDefaultBuffer(rendererContext.Device.Get(), rendererContext.CommandList.Get(),
		PassPrivates::VertexIndices.data(), IndexBufferByteSize, false, m_indexUploadBuffer);

	m_indexBufferView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = (UINT)IndexBufferByteSize;

    PassPrivates::CreateRootSignatureAndPSO(shaderLibrary, *renderer, m_rootSignature, m_pso);

    m_GBufferRTViewIndex = GBufferRTViewIndex;
}



void GraphicsPassCopyGBufferToBackbuffer::Update(int32_t frameIdxModulo, void* Data)
{

}

void GraphicsPassCopyGBufferToBackbuffer::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const
{
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	cmdList->SetPipelineState(m_pso.Get());

	cmdList->IASetIndexBuffer(&m_indexBufferView);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 0;
    const std::vector<int32_t> BindlessResourceIndices = {
        m_GBufferRTViewIndex,
        GBufferStatics::GBufferWidth,
        GBufferStatics::GBufferHeight
    };

    cmdList->SetGraphicsRoot32BitConstants(
        (UINT)BindlessResourceIndicesRootSigParamIndex,
        (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

	cmdList->DrawIndexedInstanced((UINT)PassPrivates::VertexIndices.size(), 1, 0, 0, 0);
}

void GraphicsPassCopyGBufferToBackbuffer::Shutdown()
{
}

