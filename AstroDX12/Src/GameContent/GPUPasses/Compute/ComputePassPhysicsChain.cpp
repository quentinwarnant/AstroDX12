#include "ComputePassPhysicsChain.h"

#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>

#include <Rendering\Common\MeshLibrary.h>
#include <Rendering\Renderable\IRenderable.h>
#include <Rendering\Common\FrameResource.h>

using namespace AstroTools::Rendering;

ComputePassPhysicsChain::ComputePassPhysicsChain()
    : m_frameIdxModulo(0)
{
    auto BufferDataVector = std::vector<PhysicsChain::ChainElementData>(5);
    for (int i = 0; i < 5; ++i)
    {
		BufferDataVector[i].Particle.Pos = DirectX::XMFLOAT3(10.f, i * -8.f, 10.f);
        BufferDataVector[i].RestLength = 8.0f;
        BufferDataVector[i].ParentIndex = (i > 0) ? i - 1 : -1;
        BufferDataVector[i].Pinned = (i > 0) ? false : true;
    }

    m_chainDataBufferPing = std::make_unique<StructuredBuffer<PhysicsChain::ChainElementData>>(BufferDataVector);
    m_chainDataBufferPong = std::make_unique<StructuredBuffer<PhysicsChain::ChainElementData>>(BufferDataVector);
}

void ComputePassPhysicsChain::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    {
        renderer->CreateStructuredBufferAndViews(m_chainDataBufferPing.get(), true, true);
        renderer->CreateStructuredBufferAndViews(m_chainDataBufferPong.get(), true, true);

        const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\physicsChainCompute.hlsl");

        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;
        const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Num32BitValues = 2
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
        slotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

        // Root signature is an array of root parameters
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
            (UINT)slotRootParams.size(),
            slotRootParams.data(),
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
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
        renderer->CreateRootSignature(serializedRootSignature, rootSignature);

        ComputableDesc computableObjDesc(computeShaderPath);
        computableObjDesc.RootSignature = rootSignature;

        // Create shader
        computableObjDesc.CS = shaderLibrary.GetCompiledShader(computableObjDesc.ComputeShaderPath, L"CSMain", {}, L"cs_6_6");

        // Compile PSO
        renderer->CreateComputePipelineState(
            computableObjDesc.PipelineStateObject,
            computableObjDesc.RootSignature,
            computableObjDesc.CS);

        m_particlesComputeObj = std::make_unique<ComputableObject>(computableObjDesc.RootSignature, computableObjDesc.PipelineStateObject, int16_t(0));

    }

    // TODO (schedule init particles pass)
}

void ComputePassPhysicsChain::Update(int32_t frameIdxModulo, void* /*Data*/)
{
    m_frameIdxModulo = frameIdxModulo;
}

int32_t ComputePassPhysicsChain::GetParticleReadBufferSRVHeapIndex() const
{
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();
    return bufferInput->GetSRVIndex();
}

int32_t ComputePassPhysicsChain::GetParticleOutputBufferSRVHeapIndex() const
{
    auto bufferInput = m_frameIdxModulo % 2 == 1 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();
    return bufferInput->GetSRVIndex();
}

void ComputePassPhysicsChain::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& /*frameResources*/) const
{
    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_chainDataBufferPing.get() : m_chainDataBufferPong.get();

    cmdList->SetComputeRootSignature(m_particlesComputeObj->GetRootSignature().Get());
    cmdList->SetPipelineState(m_particlesComputeObj->GetPSO().Get());

    constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 0;
    const std::vector<int32_t> BindlessResourceIndices = {
        bufferInput->GetSRVIndex(),
        bufferOutput->GetUAVIndex()
    };
    cmdList->SetComputeRoot32BitConstants(
        (UINT)BindlessResourceIndicesRootSigParamIndex,
        (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

    // Transition resources into their next correct state
    const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferInput->Resource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferOutput->Resource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
    cmdList->ResourceBarrier(2, buffersStateTransition.data());

    //TODO compute dispatch size
    cmdList->Dispatch(1, 1, 1);
}


void ComputePassPhysicsChain::Shutdown()
{
    m_chainDataBufferPing.release();
    m_chainDataBufferPong.release();

    m_particlesComputeObj.release();
}

//---------------------------------------------------------------------------------------
// Graphics Pass
//---------------------------------------------------------------------------------------

namespace Private
{
    static const std::string ChainElementMeshName = "ChainElementGeometry";
}

GraphicsPassPhysicsChain::GraphicsPassPhysicsChain()
    : m_chainElementMesh()
{
}

void GraphicsPassPhysicsChain::CreateChainElementMesh(IRenderer* renderer, MeshLibrary& meshLibrary)
{
    auto rendererContext = renderer->GetRendererContext();

    struct ChainVertexDataPOD
    {
        DirectX::XMFLOAT3 Pos;
        // TODO: add orientation
        ChainVertexDataPOD() = default;
        ChainVertexDataPOD(const DirectX::XMFLOAT3& pos)
            : Pos(pos)
        {}
	};


    std::vector<ChainVertexDataPOD> verts;
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-1.5f, 0, -1.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-1.5f, 0, 1.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+1.5f, 0, 1.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+1.5f, 0, -1.5f)));
    
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-1.5f, -3.f, -1.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-1.5f, -3.f, 1.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+1.5f, -3.f, 1.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+1.5f, -3.f, -1.5f)));

    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-2.5f, -3.f, -2.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-2.5f, -3.f, +2.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+2.5f, -3.f, +2.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+2.5f, -3.f, -2.5f)));

    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-2.5f, -8.f, -2.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(-2.5f, -8.f, +2.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+2.5f, -8.f, +2.5f)));
    verts.emplace_back(ChainVertexDataPOD(DirectX::XMFLOAT3(+2.5f, -8.f, -2.5f)));

    const std::vector<std::uint32_t> indices =
    {
        // top face
        0, 1, 2,
        2, 3, 0,

        // top left face
        0, 4, 1,
        4, 5, 1,

        // top front face
        1, 5, 2,
        5, 6, 2,

        // top Right face
        2, 6, 3,
        6, 7, 3,

        // top back face
        0, 3, 7,
        7, 4, 0,

        // mid face
        7, 11, 4,
        4, 11, 8,
        4,8,5,
        8,9,5,
        5,9,6,
        9,10,6,
        10, 7,6,
        10, 11,7,

        // bottom side faces
        8,12,9,
		12,13,9,

		9,13,10,
		13,14,10,

		10,14,11,
		14,15,11,

		11,15,8,
		15,12,8,

		// bottom face
		12,14,13,
		14,12,15
    };


    meshLibrary.AddMesh(rendererContext, Private::ChainElementMeshName, verts, indices);

}

void GraphicsPassPhysicsChain::Init(std::weak_ptr<const ComputePassPhysicsChain>  particlesComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary)
{
    m_particlesComputePass = particlesComputePass;

    const auto rootPath = s2ws(DX::GetWorkingDirectory());

    // Create the Chain Element Mesh
    CreateChainElementMesh(renderer, meshLibrary);


    // TODO: rethink this - it's ugly that we're having to do this all manually, when the RenderableStaticObject doesn't quite fit our needs (ie: we don't need to update transforms on the CPU - using GPU buffers instead)
    //      We only need the root signature, PSO, and vertex buffer (store din the mesh, not the RenderableStaticObject

    assert(meshLibrary.GetMesh(Private::ChainElementMeshName, m_chainElementMesh));
    const auto particleGraphicsShaderPath = rootPath + std::wstring(L"\\Shaders\\physicsChainGraphics.hlsl");

    auto vs = shaderLibrary.GetCompiledShader(particleGraphicsShaderPath, L"VS", {}, L"vs_6_6");
    auto ps = shaderLibrary.GetCompiledShader(particleGraphicsShaderPath, L"PS", {}, L"ps_6_6");

    std::vector<D3D12_ROOT_PARAMETER1> rootSigSlotRootParams;

    // Descriptor table of 1 CBV - the pass constants, and some root constants to access the bindless resource indices:
    // one per pass,
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
    renderer->CreateRootSignature(serializedRootSignature, m_rootSignature);

    renderer->CreateGraphicsPipelineState(
        m_pipelineStateObject,
        m_rootSignature,
        nullptr,
        vs,
        ps);
}

void GraphicsPassPhysicsChain::Update(int32_t /*frameIdxModulo*/, void* /*Data*/)
{
}

void GraphicsPassPhysicsChain::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& frameResources) const
{
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

    const int instanceCount = 5; // TODO: this is the maximum - the GPU will need to set the size of the mesh to zero for particles that are not alive
    cmdList->DrawIndexedInstanced((UINT)m_chainElementMesh.lock()->GetVertexIndicesCount(), instanceCount, 0, 0, 0);
}

void GraphicsPassPhysicsChain::Shutdown()
{
    m_chainElementMesh.reset();
    m_particlesComputePass.reset();
    m_pipelineStateObject = nullptr;
    m_pipelineStateObject = nullptr;
}

