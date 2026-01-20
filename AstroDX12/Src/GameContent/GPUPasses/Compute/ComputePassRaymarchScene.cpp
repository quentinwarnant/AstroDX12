#include "ComputePassRaymarchScene.h"

#include <Rendering/IRenderer.h>
#include <Rendering/Common/FrameResource.h>
#include <GameContent/GPUPasses/RaymarchScene.h>

using namespace AstroTools::Rendering;

namespace RaymarchScenePrivates
{
	static const int32_t ObjectCount = 10; // Number of SDF objects in the scene
}

void ComputePassRaymarchScene::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, std::weak_ptr<ComputePassParticles> particleComputePass)
{
    m_particleComputePass = particleComputePass;

    // Create SDF Scene Objects buffer
 //   auto SDFSceneObjectsList = std::vector<SDFSceneObject>(RaymarchScenePrivates::ObjectCount);
	//const float CoordRange = 15.f; // Range for random coordinates
 //   for (auto& SDFSceneObject : SDFSceneObjectsList)
 //   {
 //       SDFSceneObject.Pos = DirectX::XMFLOAT4( (float(rand() % 200) / 200) * CoordRange,
 //           (float(rand() % 123) / 123) * CoordRange,
 //           (float(rand() % 480) / 480) * CoordRange,
 //           (float(rand() % 321) / 321) * 3.f); // Size
 //   }

 //   m_SDFSceneObjectsBuffer = std::make_unique<StructuredBuffer<SDFSceneObject>>(SDFSceneObjectsList);
 //   renderer->CreateStructuredBufferAndViews(m_SDFSceneObjectsBuffer.get(), false, true);

    m_gBuffer1RT = std::make_shared<RenderTarget>();
    renderer->InitialiseRenderTarget(m_gBuffer1RT.get(), L"GBuffer1",
        GBufferStatics::GBufferWidth, GBufferStatics::GBufferHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    {
        const auto computeShaderPath = rootPath + std::wstring(L"\\Shaders\\RaymarchScene.hlsl");

        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;
        const D3D12_ROOT_PARAMETER1 rootParamCBVPerPass
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
        slotRootParams.push_back(rootParamCBVPerPass);

        const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            {
                .ShaderRegister = 1,
                .RegisterSpace = 0,
                .Num32BitValues = 5
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

        m_raymarchRootSignature = computableObjDesc.RootSignature;
        m_raymarchPSO = computableObjDesc.PipelineStateObject;
    }

}

void ComputePassRaymarchScene::Update(const GPUPassUpdateData& /*updateData*/)
{
    m_currentParticleDataBufferSRVIdx = m_particleComputePass.lock()->GetParticleOutputBufferSRVHeapIndex();
}

void ComputePassRaymarchScene::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "ComputePassRaymarchScene");

    const auto GBufferRTPreDrawStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_gBuffer1RT->GetResource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersPreDrawStateTransition = { GBufferRTPreDrawStateTransition };
    cmdList->ResourceBarrier(1, buffersPreDrawStateTransition.data());


    cmdList->SetComputeRootSignature(m_raymarchRootSignature.Get());
    cmdList->SetPipelineState(m_raymarchPSO.Get());
    
    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    cmdList->SetComputeRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);

    constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 1;
    const std::vector<int32_t> BindlessResourceIndices = {
        m_currentParticleDataBufferSRVIdx,
        m_gBuffer1RT->GetUAVIndex(),
        RaymarchScenePrivates::ObjectCount,
        GBufferStatics::GBufferWidth,
		GBufferStatics::GBufferHeight    
    };
    cmdList->SetComputeRoot32BitConstants(
        (UINT)BindlessResourceIndicesRootSigParamIndex,
        (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);


	int32_t DispatchX = GBufferStatics::GBufferWidth / 8; // 8 threads per group in X
	int32_t DispatchY = GBufferStatics::GBufferHeight / 8; // 8 threads per group in Y
    cmdList->Dispatch(DispatchX, DispatchY, 1);

    const auto GBufferRTPostDrawStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_gBuffer1RT->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersPostDrawStateTransition = { GBufferRTPostDrawStateTransition };
    cmdList->ResourceBarrier(1, buffersPostDrawStateTransition.data());
}

void ComputePassRaymarchScene::Shutdown()
{
}