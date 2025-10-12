#include "ComputePassParticles.h"

#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>

#include <Rendering\Common\MeshLibrary.h>
#include <Rendering\Renderable\IRenderable.h>
#include <Rendering\Common\FrameResource.h>

using namespace AstroTools::Rendering;

ComputePassParticles::ComputePassParticles()
    : m_frameIdxModulo(0)
{
    auto BufferDataVector = std::vector<ParticleData>(20);
    for (auto& ParticleData : BufferDataVector)
    {
        ParticleData.Duration = 3.f + (float(rand() % 200) / 200);
        ParticleData.Size = 2.f;
    }

    m_particleDataBufferPing = std::make_unique<StructuredBuffer<ParticleData>>(BufferDataVector);
    m_particleDataBufferPong = std::make_unique<StructuredBuffer<ParticleData>>(BufferDataVector);
}

void ComputePassParticles::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    {
        renderer->CreateStructuredBufferAndViews(m_particleDataBufferPing.get(), true, true);
        renderer->CreateStructuredBufferAndViews(m_particleDataBufferPong.get(), true, true);

        const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\particles.hlsl");

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

void ComputePassParticles::Update(int32_t frameIdxModulo, void* /*Data*/)
{
    m_frameIdxModulo = frameIdxModulo;
}

int32_t ComputePassParticles::GetParticleReadBufferSRVHeapIndex() const
{
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_particleDataBufferPing.get() : m_particleDataBufferPong.get();
    return bufferInput->GetSRVIndex();
}

int32_t ComputePassParticles::GetParticleOutputBufferSRVHeapIndex() const
{
    auto bufferInput = m_frameIdxModulo % 2 == 1 ? m_particleDataBufferPing.get() : m_particleDataBufferPong.get();
    return bufferInput->GetSRVIndex();
}

void ComputePassParticles::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& /*frameResources*/) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "ComputePassParticles");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_particleDataBufferPing.get() : m_particleDataBufferPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_particleDataBufferPing.get() : m_particleDataBufferPong.get();

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
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE , D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
    cmdList->ResourceBarrier(2, buffersStateTransition.data());

    //TODO compute dispatch size
	cmdList->Dispatch(1, 1, 1);
}


void ComputePassParticles::Shutdown()
{
    m_particleDataBufferPing.release();
    m_particleDataBufferPong.release();

    m_particlesComputeObj.release();
}

//---------------------------------------------------------------------------------------
// Graphics Pass
//---------------------------------------------------------------------------------------

GraphicsPassParticles::GraphicsPassParticles()
    : m_boxMesh()
{
}

void GraphicsPassParticles::Init(std::weak_ptr<const ComputePassParticles>  particlesComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, const MeshLibrary& meshLibrary)
{
    m_particlesComputePass = particlesComputePass;

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    
    // TODO: rethink this - it's ugly that we're having to do this all manually, when the RenderableStaticObject doesn't quite fit our needs (ie: we don't need to update transforms on the CPU - using GPU buffers instead)
    //      We only need the root signature, PSO, and vertex buffer (store din the mesh, not the RenderableStaticObject

    assert(meshLibrary.GetMesh(std::string("BoxGeometry"), m_boxMesh));
    const auto particleGraphicsShaderPath = rootPath + std::wstring(L"\\Shaders\\particleGraphics.hlsl");

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

void GraphicsPassParticles::Update(int32_t /*frameIdxModulo*/, void* /*Data*/)
{
}

void GraphicsPassParticles::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "GraphicsPassParticles");

    const auto indexBuffer = m_boxMesh.lock()->IndexBufferView();
    cmdList->IASetIndexBuffer(&indexBuffer);
    cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pipelineStateObject.Get());

    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);

    const uint32_t GraphicsBindlessResourceIndicesRootSigParamIndex = 1;
    const std::vector<int32_t> GraphicsBindlessResourceIndices = {
        m_particlesComputePass.lock()->GetParticleReadBufferSRVHeapIndex(), 
        m_boxMesh.lock()->GetVertexBufferSRV()                             
    };                                                                      
    cmdList->SetGraphicsRoot32BitConstants(
        (UINT)GraphicsBindlessResourceIndicesRootSigParamIndex,
        (UINT)GraphicsBindlessResourceIndices.size(), GraphicsBindlessResourceIndices.data(), 0);

    const int particleCount = 20; // TODO: this is the maximum - the GPU will need to set the size of the mesh to zero for particles that are not alive
    cmdList->DrawIndexedInstanced((UINT)m_boxMesh.lock()->GetVertexIndicesCount(), particleCount, 0, 0, 0);
}

void GraphicsPassParticles::Shutdown()
{
    m_boxMesh.reset();
    m_particlesComputePass.reset();
    m_pipelineStateObject = nullptr;
    m_pipelineStateObject = nullptr;
}

