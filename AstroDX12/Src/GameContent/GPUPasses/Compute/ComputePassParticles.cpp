#include "ComputePassParticles.h"

#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>

using namespace AstroTools::Rendering;

ComputePassParticles::ComputePassParticles()
    : m_frameIdx(0)
{
    auto BufferDataVector = std::vector<ParticleData>(20);

    m_particleDataBufferPing = std::make_unique<StructuredBuffer<ParticleData>>(BufferDataVector);
    m_particleDataBufferPong = std::make_unique<StructuredBuffer<ParticleData>>(BufferDataVector);
}

void ComputePassParticles::Init(IRenderer& renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    renderer.CreateComputableObjStructuredBufferAndViews(m_particleDataBufferPing.get(), true, true);
    renderer.CreateComputableObjStructuredBufferAndViews(m_particleDataBufferPong.get(), true, true);

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\particles.hlsl");
    const std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout
    {
        { "ParticlesDataIn", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0 },
        { "ParticlesDataOut", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(ParticleData), D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 0}
        
    };

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
    renderer.CreateRootSignature(serializedRootSignature, rootSignature);

    ComputableDesc computableObjDesc(computeShaderPath, InputLayout);
    computableObjDesc.RootSignature = rootSignature;

    // Create shader
    computableObjDesc.CS = shaderLibrary.GetCompiledShader(computableObjDesc.ComputeShaderPath, L"CSMain", {}, L"cs_6_6");
    
    // Compile PSO
    renderer.CreateComputePipelineState(
        computableObjDesc.PipelineStateObject,
        computableObjDesc.RootSignature,
        computableObjDesc.InputLayout,
        computableObjDesc.CS);

    m_particlesComputeObj = std::make_unique<ComputableObject>(computableObjDesc.RootSignature, computableObjDesc.PipelineStateObject, int16_t(0));

    // TODO (schedule init particles pass)
}

void ComputePassParticles::Update(void* /*Data*/)
{
    m_frameIdx++;
}

void ComputePassParticles::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& /*frameResources*/,
    const DescriptorHeap& descriptorHeap,
    int32_t /*descriptorSizeCBV*/) const
{
    // Dispatch Particle update
    auto bufferInput = m_frameIdx % 2 == 0 ? m_particleDataBufferPing.get() : m_particleDataBufferPong.get();
    auto bufferOutput = m_frameIdx % 2 == 1 ? m_particleDataBufferPing.get() : m_particleDataBufferPong.get();

	cmdList->SetComputeRootSignature(m_particlesComputeObj->GetRootSignature().Get());
	cmdList->SetPipelineState(m_particlesComputeObj->GetPSO().Get());

	auto computePassDescriptorHeapHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptorHeap.GetGPUDescriptorHandleForHeapStart());
    constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 0;
    const std::vector<int32_t> BindlessResourceIndices = {
        bufferInput->GetSRVIndex(),
        bufferOutput->GetUAVIndex()
    };
    cmdList->SetComputeRoot32BitConstants(
        (UINT)BindlessResourceIndicesRootSigParamIndex,
        (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

    // Transition resources into correct state
    const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferInput->Resource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferOutput->Resource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

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