#include "ComputePassVBDChain.h"

#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>

#include <Rendering\Common\MeshLibrary.h>
#include <Rendering\Renderable\IRenderable.h>
#include <Rendering\Common\FrameResource.h>

#include <pix3.h>

using namespace AstroTools::Rendering;
using namespace DirectX;

namespace Privates_VBD
{
    static const size_t NumChainElements = 15; // Keep in sync with hlsl file
}

ComputePassVBDChain::ComputePassVBDChain()
    : m_frameIdxModulo(0)
	, m_simNeedsReset(false)
{
    auto BufferDataVector = std::vector<VBDChain::ChainElementData>(Privates_VBD::NumChainElements);

	XMVECTOR nodePos = DirectX::XMVectorSet(10.f, 0.f, 10.f, 0.f);
	XMVECTOR nodeOffset = DirectX::XMVectorSet(0.f, -8.f, 0.f, 0.f);
    XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
    for (int i = 0; i < Privates_VBD::NumChainElements; ++i)
    {
        DirectX::XMStoreFloat3( &BufferDataVector[i].Particle.Pos, nodePos);
        DirectX::XMStoreFloat3( &BufferDataVector[i].Particle.PrevPos, nodePos);

        rotation = DirectX::XMMatrixRotationRollPitchYaw(0.f, 0.f, i * 10.f * 3.14f / 180.f);
		DirectX::XMStoreFloat3x3(&BufferDataVector[i].Particle.Rot,  DirectX::XMMatrixInverse(nullptr, rotation));

        XMVECTOR nodeOffsetRotated = DirectX::XMVector3TransformNormal(nodeOffset, rotation);
        XMVECTOR nextNodePos = DirectX::XMVectorAdd(nodePos, nodeOffsetRotated);
        nodePos = nextNodePos;
        
        BufferDataVector[i].RestLength = 8.0f;
        BufferDataVector[i].ParentIndex = (i > 0) ? i - 1 : -1;
        BufferDataVector[i].Pinned = (i > 0) ? false : true;
        BufferDataVector[i].Radius = 2.f;
    }

    m_chainDataBufferPing = std::make_unique<StructuredBuffer<VBDChain::ChainElementData>>(BufferDataVector);
    m_chainDataBufferPong = std::make_unique<StructuredBuffer<VBDChain::ChainElementData>>(BufferDataVector);
}

void ComputePassVBDChain::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, int32_t debugDrawBufferUAVIndex, int32_t debugCounterBufferUAVIndex)
{
    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    {
        renderer->CreateStructuredBufferAndViews(m_chainDataBufferPing.get(), std::wstring_view(L"VBDChainData_Ping"), true, true);
        renderer->CreateStructuredBufferAndViews(m_chainDataBufferPong.get(), std::wstring_view(L"VBDChainData_Pong"), true, true);

        const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\vbdChainCompute.hlsl");

        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;
        const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Num32BitValues = 5
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
        slotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
            (UINT)slotRootParams.size(),
            slotRootParams.data(),
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
        );

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
        renderer->CreateRootSignature(serializedRootSignature, rootSignature);

        ComputableDesc computableObjDesc(computeShaderPath);
        computableObjDesc.RootSignature = rootSignature;

        computableObjDesc.CS = shaderLibrary.GetCompiledShader(computableObjDesc.ComputeShaderPath, L"CSMain", {}, L"cs_6_6");

        renderer->CreateComputePipelineState(
            computableObjDesc.PipelineStateObject,
            computableObjDesc.RootSignature,
            computableObjDesc.CS);

        m_particlesComputeObj = std::make_unique<ComputableObject>(computableObjDesc.RootSignature, computableObjDesc.PipelineStateObject);
    }

    m_debugDrawBufferUAVIndex = debugDrawBufferUAVIndex;
    m_debugDrawCounterUAVIndex = debugCounterBufferUAVIndex;
}

void ComputePassVBDChain::Update(const GPUPassUpdateData& updateData)
{
    m_frameIdxModulo = updateData.frameIdxModulo;

    m_simNeedsReset.Tick();
}

int32_t ComputePassVBDChain::GetParticleReadBufferSRVHeapIndex() const
{
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();
    return bufferInput->GetSRVIndex();
}

int32_t ComputePassVBDChain::GetParticleOutputBufferSRVHeapIndex() const
{
    auto bufferInput = m_frameIdxModulo % 2 == 1 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();
    return bufferInput->GetSRVIndex();
}

void ComputePassVBDChain::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& /*frameResources*/) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(128, 0, 255), "ComputePassVBDChain");

    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();

    cmdList->SetComputeRootSignature(m_particlesComputeObj->GetRootSignature().Get());
    cmdList->SetPipelineState(m_particlesComputeObj->GetPSO().Get());

    constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 0;
    const std::vector<int32_t> BindlessResourceIndices = {
        bufferInput->GetSRVIndex(),
        bufferOutput->GetUAVIndex(),
        m_simNeedsReset.NeedsReset() ? 1 : 0,
        m_debugDrawBufferUAVIndex,
        m_debugDrawCounterUAVIndex
    };
    cmdList->SetComputeRoot32BitConstants(
        (UINT)BindlessResourceIndicesRootSigParamIndex,
        (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

    const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferInput->Resource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferOutput->Resource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
    cmdList->ResourceBarrier(2, buffersStateTransition.data());

    cmdList->Dispatch(1, 1, 1);
}


void ComputePassVBDChain::Shutdown()
{
}

//---------------------------------------------------------------------------------------
// Graphics Pass
//---------------------------------------------------------------------------------------

namespace Private_VBD
{
    static const std::string ChainElementMeshName = "VBDChainElementGeometry";
}

GraphicsPassVBDChain::GraphicsPassVBDChain()
    : m_chainElementMesh()
{
}

void GraphicsPassVBDChain::CreateChainElementMesh(IRenderer* renderer, MeshLibrary& meshLibrary)
{
    struct ChainVertexDataPOD
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        ChainVertexDataPOD() = default;
        ChainVertexDataPOD(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& normal)
            : Pos(pos)
            , Normal(normal)
        {}
	};


    std::vector<ChainVertexDataPOD> verts;
    verts.emplace_back(DirectX::XMFLOAT3(-1.5f, 0, -1.5f), DirectX::XMFLOAT3(0, 1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(-1.5f, 0, 1.5f), DirectX::XMFLOAT3(0, 1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(+1.5f, 0, 1.5f), DirectX::XMFLOAT3(0, 1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(+1.5f, 0, -1.5f), DirectX::XMFLOAT3(0, 1, 0));

    verts.emplace_back(DirectX::XMFLOAT3(-1.5f, -3.f, -1.5f), DirectX::XMFLOAT3(-1, 0, -1));
    verts.emplace_back(DirectX::XMFLOAT3(-1.5f, -3.f, 1.5f), DirectX::XMFLOAT3(-1, 0, 1));
    verts.emplace_back(DirectX::XMFLOAT3(+1.5f, -3.f, 1.5f), DirectX::XMFLOAT3(1, 0, 1));
    verts.emplace_back(DirectX::XMFLOAT3(+1.5f, -3.f, -1.5f), DirectX::XMFLOAT3(1, 0, -1));

    verts.emplace_back(DirectX::XMFLOAT3(-2.5f, -3.f, -2.5f), DirectX::XMFLOAT3(0, 1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(-2.5f, -3.f, +2.5f), DirectX::XMFLOAT3(0, 1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(+2.5f, -3.f, +2.5f), DirectX::XMFLOAT3(0, 1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(+2.5f, -3.f, -2.5f), DirectX::XMFLOAT3(0, 1, 0));

    verts.emplace_back(DirectX::XMFLOAT3(-2.5f, -8.f, -2.5f), DirectX::XMFLOAT3(0, -1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(-2.5f, -8.f, +2.5f), DirectX::XMFLOAT3(0, -1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(+2.5f, -8.f, +2.5f), DirectX::XMFLOAT3(0, -1, 0));
    verts.emplace_back(DirectX::XMFLOAT3(+2.5f, -8.f, -2.5f), DirectX::XMFLOAT3(0, -1, 0));

    const std::vector<std::uint32_t> indices =
    {
        0, 1, 2,
        2, 3, 0,

        0, 4, 1,
        4, 5, 1,

        1, 5, 2,
        5, 6, 2,

        2, 6, 3,
        6, 7, 3,

        0, 3, 7,
        7, 4, 0,

        7, 11, 4,
        4, 11, 8,
        4,8,5,
        8,9,5,
        5,9,6,
        9,10,6,
        10, 7,6,
        10, 11,7,

        8,12,9,
		12,13,9,

		9,13,10,
		13,14,10,

		10,14,11,
		14,15,11,

		11,15,8,
		15,12,8,

		12,14,13,
		14,12,15
    };


    meshLibrary.AddMesh(renderer->GetRendererContext(), Private_VBD::ChainElementMeshName, verts, indices);
}

void GraphicsPassVBDChain::Init(std::weak_ptr<const ComputePassVBDChain> particlesComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary)
{
    m_particlesComputePass = particlesComputePass;

    const auto rootPath = s2ws(DX::GetWorkingDirectory());

    CreateChainElementMesh(renderer, meshLibrary);

    assert(meshLibrary.GetMesh(Private_VBD::ChainElementMeshName, m_chainElementMesh));
    const auto graphicsShaderPath = rootPath + std::wstring(L"\\Shaders\\vbdChainGraphics.hlsl");

    auto vs = shaderLibrary.GetCompiledShader(graphicsShaderPath, L"VS", {}, L"vs_6_6");
    auto ps = shaderLibrary.GetCompiledShader(graphicsShaderPath, L"PS", {}, L"ps_6_6");

    std::vector<D3D12_ROOT_PARAMETER1> rootSigSlotRootParams;

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
    rootSigSlotRootParams.push_back(rootParamCBVPerPass);

    const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
    {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
        .Constants
        {
            .ShaderRegister = 1,
            .RegisterSpace = 0,
            .Num32BitValues = 2
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
    };
    rootSigSlotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
        (UINT)rootSigSlotRootParams.size(),
        rootSigSlotRootParams.data(),
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
    );

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
    renderer->CreateRootSignature(serializedRootSignature, m_rootSignature);

    renderer->CreateGraphicsPipelineState(
        m_pipelineStateObject,
        m_rootSignature,
        nullptr,
        vs,
        ps);
}

void GraphicsPassVBDChain::Update(const GPUPassUpdateData& /*updateData*/)
{
}

void GraphicsPassVBDChain::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(128, 0, 255), "GraphicsPassVBDChain");

    const auto indexBuffer = m_chainElementMesh.lock()->IndexBufferView();
    cmdList->IASetIndexBuffer(&indexBuffer);
    cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pipelineStateObject.Get());

    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);

    const uint32_t GraphicsBindlessResourceIndicesRootSigParamIndex = 1;
    const std::vector<int32_t> GraphicsBindlessResourceIndices = {
        m_particlesComputePass.lock()->GetParticleReadBufferSRVHeapIndex(),
        m_chainElementMesh.lock()->GetVertexBufferSRV()
    };
    cmdList->SetGraphicsRoot32BitConstants(
        (UINT)GraphicsBindlessResourceIndicesRootSigParamIndex,
        (UINT)GraphicsBindlessResourceIndices.size(), GraphicsBindlessResourceIndices.data(), 0);

    const int instanceCount = Privates_VBD::NumChainElements;
    cmdList->DrawIndexedInstanced((UINT)m_chainElementMesh.lock()->GetVertexIndicesCount(), instanceCount, 0, 0, 0);
}

void GraphicsPassVBDChain::Shutdown()
{
}
