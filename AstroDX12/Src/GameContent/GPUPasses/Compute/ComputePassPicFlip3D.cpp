#include "ComputePassPicFlip3D.h"

#include "Rendering\Common\FrameResource.h"
#include "Rendering\RenderData\VertexData.h"
#include "Rendering/RenderData/GeometryHelper.h"
#include <Rendering/Common/VectorTypes.h>
#include <cassert>

namespace Privates
{
    int32_t ParticleCount = 1000;
    const std::string MeshName = "Sphere";
	ivec3 GridResolution = ivec3(32, 32, 32);
}

void ComputePassPicFlip3D::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    auto BufferDataVector = std::vector<PicFlip::ParticleData>(Privates::ParticleCount);
    int32_t index = 0;
    for (auto& ParticleData : BufferDataVector)
    {
        float x = (float(index % 10) * 3.f);
        ParticleData.Pos = XMFLOAT3(x, (index / 100) * 2.f, ((index % 100) / 10) * 3.f);
        ParticleData.Vel = XMFLOAT3(0.f, 0.f, 0.f);
        index++;
    }

    m_particleDataBufferPair = std::make_unique<PicFlip::ParticleDataBufferPair>(
        std::make_shared<StructuredBuffer<PicFlip::ParticleData>>(BufferDataVector),
        std::make_shared<StructuredBuffer<PicFlip::ParticleData>>(BufferDataVector)
    );

    renderer->CreateStructuredBufferAndViews(m_particleDataBufferPair->GetInput(), true, true);
    renderer->CreateStructuredBufferAndViews(m_particleDataBufferPair->GetOutput(), true, true);
    
    m_pressureGridPair = std::make_unique<PicFlip::GridDataBufferPair>(
        std::make_shared<Texture3D>(),
        std::make_shared<Texture3D>()
    );
    
    //TODO: put the resources into the right resource state?
    // m_particleDataBufferPair
    // 


	// Init pressure grid 3D textures
    {
        auto InitPressureGrid = [](IRenderer* renderer, ITexture3D* texture3D, std::wstring name, D3D12_RESOURCE_STATES initialResourceState)
        {
            renderer->InitialiseTexture3D(*texture3D, true,
                name,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                Privates::GridResolution.x, Privates::GridResolution.y, Privates::GridResolution.z,
                initialResourceState,
                /*miplevels*/ 0,
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 
                D3D12_TEXTURE_LAYOUT_UNKNOWN);
        };
        InitPressureGrid(renderer, m_pressureGridPair->GetInput(), L"PicFlip3D_PressureGrid_Ping", D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        InitPressureGrid(renderer, m_pressureGridPair->GetOutput(), L"PicFlip3D_PressureGrid_Pong", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    }

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    {
        const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\LagrangianFluidSim\\Simulate.hlsl");

        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;
        const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Num32BitValues = 4
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

        m_particlesComputeObj = std::make_unique<ComputableObject>(computableObjDesc.RootSignature, computableObjDesc.PipelineStateObject);
    }
}

int32_t ComputePassPicFlip3D::GetParticleReadBufferSRVHeapIndex() const
{
    return m_particleDataBufferPair->GetInput()->GetSRVIndex();
}

int32_t ComputePassPicFlip3D::GetParticleOutputBufferSRVHeapIndex() const
{
    return m_particleDataBufferPair->GetOutput()->GetSRVIndex();
}

void ComputePassPicFlip3D::Update(const GPUPassUpdateData& /*updateData*/)
{
    m_particleDataBufferPair->Swap();
    m_pressureGridPair->Swap();
}

void ComputePassPicFlip3D::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "ComputePassPicFlip3D");

    // Dispatch Particle update

    cmdList->SetComputeRootSignature(m_particlesComputeObj->GetRootSignature().Get());
    cmdList->SetPipelineState(m_particlesComputeObj->GetPSO().Get());

    constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 0;
    const std::vector<int32_t> BindlessResourceIndices = {
        m_particleDataBufferPair->GetInput()->GetSRVIndex(),
        m_particleDataBufferPair->GetOutput()->GetUAVIndex(),
        m_pressureGridPair->GetInput()->GetSRVIndex(),
        m_pressureGridPair->GetOutput()->GetUAVIndex()
    };
    cmdList->SetComputeRoot32BitConstants(
        (UINT)BindlessResourceIndicesRootSigParamIndex,
        (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

    //TODO compute dispatch size
    cmdList->Dispatch(1, 1, 1);


    // Transition resources into their next correct state
    const auto particleBufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_particleDataBufferPair->GetInput()->Resource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    const auto particleBufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_particleDataBufferPair->GetOutput()->Resource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const auto pressureGridTextureInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pressureGridPair->GetInput()->Resource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    const auto pressureGridTextureOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_pressureGridPair->GetOutput()->Resource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);


    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { 
        particleBufferInStateTransition,
        particleBufferOutStateTransition,
        pressureGridTextureInStateTransition,
        pressureGridTextureOutStateTransition
    };
    cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());
}

void ComputePassPicFlip3D::Shutdown()
{
}


// ---------------------- Graphics Pass ---------------------------


void GraphicsPassPicFlip3D::Init(std::weak_ptr<const ComputePassPicFlip3D> fluidSimComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary)
{
	m_fluidSimComputePass = fluidSimComputePass;

    assert(meshLibrary.GetMesh(Privates::MeshName, m_sphereMesh));
    
    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    const auto particleGraphicsShaderPath = rootPath + std::wstring(L"\\Shaders\\LagrangianFluidSim\\Render.hlsl");

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
            .Num32BitValues = 4
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

void GraphicsPassPicFlip3D::Update(const GPUPassUpdateData& updateData)
{
}

void GraphicsPassPicFlip3D::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "GraphicsPassFluidSim2D");

    const auto indexBuffer = m_sphereMesh.lock()->IndexBufferView();
    cmdList->IASetIndexBuffer(&indexBuffer);
    cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pipelineStateObject.Get());

    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);

    const uint32_t GraphicsBindlessResourceIndicesRootSigParamIndex = 1;
    const std::vector<int32_t> GraphicsBindlessResourceIndices = {
        m_fluidSimComputePass.lock()->GetParticleOutputBufferSRVHeapIndex(),
        m_sphereMesh.lock()->GetVertexBufferSRV()
    };
    cmdList->SetGraphicsRoot32BitConstants(
        (UINT)GraphicsBindlessResourceIndicesRootSigParamIndex,
        (UINT)GraphicsBindlessResourceIndices.size(), GraphicsBindlessResourceIndices.data(), 0);

    cmdList->DrawIndexedInstanced((UINT)m_sphereMesh.lock()->GetVertexIndicesCount(), Privates::ParticleCount, 0, 0, 0);
}

void GraphicsPassPicFlip3D::Shutdown()
{
}
