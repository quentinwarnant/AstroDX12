#include "ComputePassFluidSim2D.h"
#include <Rendering/Common/VectorTypes.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/Common/FrameResource.h>
#include <Rendering/Common/SamplerIDs.h>

namespace Privates
{
	constexpr ivec2 ThreadGroupSize = ivec2(32, 32);
    constexpr ivec2 GridDimensions = ivec2(256, 256);

    std::unique_ptr<ComputableObject> CreateComputableObject(
        IRenderer* renderer,
        AstroTools::Rendering::ShaderLibrary& shaderLibrary,
        const std::wstring& computeShaderPath,
        const std::wstring& entryPoint,
        UINT numRenderTargetInputs,
        UINT numBindlessConstants)
    {
        std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;

        // SRV descriptor table for Texture2D (BufferIn)
        D3D12_DESCRIPTOR_RANGE1 srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0; // t0
        srvRange.RegisterSpace = 0;
        srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        for (size_t i = 0; i < numRenderTargetInputs; ++i)
        {
            ranges.push_back(srvRange);
            ranges[i].BaseShaderRegister = (UINT)i;
        }

        // UAV descriptor table for RWTexture2D (BufferOut)
        D3D12_DESCRIPTOR_RANGE1 uavRange = {};
        uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRange.NumDescriptors = 1;
        uavRange.BaseShaderRegister = 0; // u0
        uavRange.RegisterSpace = 0;
        uavRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        //ranges.push_back(uavRange);

        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

        // SRV table
        for (size_t i = 0; i < numRenderTargetInputs; ++i)
        {
            D3D12_ROOT_PARAMETER1 srvTable = {};
            srvTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            srvTable.DescriptorTable.NumDescriptorRanges = 1;
            srvTable.DescriptorTable.pDescriptorRanges = &ranges[i];
            srvTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            slotRootParams.push_back(srvTable);
        }

        // UAV table
        D3D12_ROOT_PARAMETER1 uavTable = {};
        uavTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        uavTable.DescriptorTable.NumDescriptorRanges = 1;
        uavTable.DescriptorTable.pDescriptorRanges = &uavRange;
        uavTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        slotRootParams.push_back(uavTable);

        // constants
        D3D12_ROOT_PARAMETER1 rootConstants
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            { 
                .ShaderRegister = 0, 
                .RegisterSpace = 0,
                .Num32BitValues = numBindlessConstants
            },

            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };
        slotRootParams.push_back(rootConstants);

        // Root signature is an array of root parameters
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
            (UINT)slotRootParams.size(),
            slotRootParams.data(),
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_NONE
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
        computableObjDesc.CS = shaderLibrary.GetCompiledShader(computableObjDesc.ComputeShaderPath, entryPoint, {}, L"cs_6_6");

        // Compile PSO
        renderer->CreateComputePipelineState(
            computableObjDesc.PipelineStateObject,
            computableObjDesc.RootSignature,
            computableObjDesc.CS);

        return std::make_unique<ComputableObject>(computableObjDesc.RootSignature, computableObjDesc.PipelineStateObject);
    }

    constexpr ivec2 ComputeDispatchSize(ivec2 dataSize, ivec2 threadGroupSize)
    {
        const int32_t dispatchX = (dataSize.x + (threadGroupSize.x-1)) / threadGroupSize.x;
        const int32_t dispatchY = (dataSize.y + (threadGroupSize.y-1)) / threadGroupSize.y;
		return ivec2(dispatchX, dispatchY);
	}

    void ApplyRootSignatureAndPSO(ComPtr<ID3D12GraphicsCommandList> cmdList, ComputableObject* computableObj)
    {
        cmdList->SetComputeRootSignature(computableObj->GetRootSignature().Get());
        cmdList->SetPipelineState(computableObj->GetPSO().Get());
	}
}

void ComputePassFluidSim2D::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    m_dummySRVGPUHandle = renderer->GetDummySRVGPUHandle();

    m_gridVelocityTexPair = std::make_unique<RenderTargetPair>(
        std::make_unique<RenderTarget>(),
		std::make_unique<RenderTarget>());
    renderer->InitialiseRenderTarget(m_gridVelocityTexPair->GetInput(), L"FluidSim2D::VelocityGrid::Ping", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32G32_FLOAT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    renderer->InitialiseRenderTarget(m_gridVelocityTexPair->GetOutput(), L"FluidSim2D::VelocityGrid::Pong", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32G32_FLOAT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    m_gridDivergenceTex = std::make_unique<RenderTarget>();
    renderer->InitialiseRenderTarget(m_gridDivergenceTex.get(), L"FluidSim2D::DivergenceGrid", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    m_gridPressureTexPair = std::make_unique<RenderTargetPair>(
        std::make_unique<RenderTarget>(),
        std::make_unique<RenderTarget>());
    renderer->InitialiseRenderTarget(m_gridPressureTexPair->GetInput(), L"FluidSim2D::PressureGrid::Ping", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    renderer->InitialiseRenderTarget(m_gridPressureTexPair->GetOutput(), L"FluidSim2D::PressureGrid::Pong", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_imageRenderTarget = std::make_unique<RenderTarget>();
	renderer->InitialiseRenderTarget(m_imageRenderTarget.get(), L"FluidSim2D::ImageRT", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32G32_FLOAT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    const auto computeShaderPathInput = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Input.hlsl");
    m_computeObjInput = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathInput, L"CSMain", 1, 1 );

    const auto computeShaderPathAdvect = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Advect.hlsl");
    m_computeObjAdvect = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathAdvect, L"CSMain", 1, 1);

    const auto computeShaderPathDivergence = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Div.hlsl");
    m_computeObjDivergence = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathDivergence, L"CSMain", 1, 1);

    //const auto computeShaderPathGradient = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Grad.hlsl");
    //m_computeObjGradient = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathGradient);

    const auto computeShaderPathDiffuse = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Diffuse.hlsl");
    m_computeObjDiffuse = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathDiffuse, L"CSMain", 1, 1);

    const auto computeShaderPathPressure = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Pressure.hlsl");
    m_computeObjPressure = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathPressure, L"CSMain", 2, 1);
    m_computeObjPressureFixEdges = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathPressure, L"CSMain_FixEdges", 2, 1);

    const auto computeShaderPathProject = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Project.hlsl");
    m_computeObjProject = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathProject, L"CSMain", 2, 1);
    m_computeObjReflectEdgeVelocity = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathProject, L"CSMain_ReflectEdgeVelocity", 2, 1);
}

void ComputePassFluidSim2D::Update(int32_t /*frameIdxModulo*/, void* /*Data*/)
{
    m_gridVelocityTexPair->Swap();
}

void ComputePassFluidSim2D::FluidStepInput(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepInput");

    // Dispatch Particle update
    auto bufferInput = m_gridVelocityTexPair->GetInput();
    auto bufferOutput = m_gridVelocityTexPair->GetOutput();

    // Transition resources into their next correct state
    {    
        const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            bufferInput->GetResource(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            bufferOutput->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
        cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());
    }
    Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjInput.get());

    cmdList->SetComputeRootDescriptorTable(0, bufferInput->GetSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, bufferOutput->GetUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstant(2, Privates::GridDimensions.x, 0); // GridWidth


    constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
    cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);


    // Transition resources into their next correct state
    {

        const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            bufferInput->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            bufferOutput->GetResource(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
        cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());
    }
	m_gridVelocityTexPair->Swap();
}

void ComputePassFluidSim2D::FluidStepDiv(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepDiv");

    auto velocityInput = m_gridVelocityTexPair->GetInput();
    auto divergenceTex= m_gridDivergenceTex.get();

    Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjDivergence.get());

    cmdList->SetComputeRootDescriptorTable(0, velocityInput->GetSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, divergenceTex->GetUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstant(2, Privates::GridDimensions.x, 0); // GridWidth

    constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
    cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);

    {
        const auto divergenceBufferStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            divergenceTex->GetResource(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { divergenceBufferStateTransition };
        cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());
    }
}

void ComputePassFluidSim2D::FluidStepPressure(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepPressure");

	const float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmdList->ClearUnorderedAccessViewFloat(
        m_gridPressureTexPair->GetOutput()->GetUAVGPUDescriptorHandle(),
        m_gridPressureTexPair->GetOutput()->GetUAVCPUDescriptorHandle(),
        m_gridPressureTexPair->GetOutput()->GetResource(),
        clearValues,
        0,
		nullptr);

    auto divTex = m_gridDivergenceTex.get();

    Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjPressure.get());
    constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
	constexpr int32_t pressureIterations = 60;
    cmdList->SetComputeRootDescriptorTable(0, divTex->GetSRVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstant(3, Privates::GridDimensions.x, 0); // GridWidth

    for (int32_t i = 0; i < pressureIterations; ++i)
    {
        auto pressureTexInput = m_gridPressureTexPair->GetInput();
        auto pressureTexOutput = m_gridPressureTexPair->GetOutput();

        cmdList->SetComputeRootDescriptorTable(1, pressureTexInput->GetSRVGPUDescriptorHandle());
        cmdList->SetComputeRootDescriptorTable(2, pressureTexOutput->GetUAVGPUDescriptorHandle());

        cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);

        // Swap input and output for next iteration
        const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            pressureTexInput->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            pressureTexOutput->GetResource(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        
        std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
        cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());

        m_gridPressureTexPair->Swap();
	}


	// - Fix edges
    {

        Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjPressureFixEdges.get());

        auto pressureTexInput = m_gridPressureTexPair->GetInput();
        auto pressureTexOutput = m_gridPressureTexPair->GetOutput();

        cmdList->SetComputeRootDescriptorTable(0, m_dummySRVGPUHandle);
        cmdList->SetComputeRootDescriptorTable(1, pressureTexInput->GetSRVGPUDescriptorHandle());
        cmdList->SetComputeRootDescriptorTable(2, pressureTexOutput->GetUAVGPUDescriptorHandle());
        cmdList->SetComputeRoot32BitConstant(3, Privates::GridDimensions.x, 0); // GridWidth
        cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);


        const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            pressureTexInput->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            pressureTexOutput->GetResource(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        const auto divergenceStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            divTex->GetResource(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);



        std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition, divergenceStateTransition };
        cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());

        m_gridPressureTexPair->Swap();
    }

}

void ComputePassFluidSim2D::FluidStepProject(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepProject");

    auto velocityInTex = m_gridVelocityTexPair->GetInput();
    auto velocityOutTex = m_gridVelocityTexPair->GetOutput();
    auto pressureTex = m_gridPressureTexPair->GetInput();

    {
        Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjProject.get());

        cmdList->SetComputeRootDescriptorTable(0, velocityInTex->GetSRVGPUDescriptorHandle());
        cmdList->SetComputeRootDescriptorTable(1, pressureTex->GetSRVGPUDescriptorHandle());
        cmdList->SetComputeRootDescriptorTable(2, velocityOutTex->GetUAVGPUDescriptorHandle());
        cmdList->SetComputeRoot32BitConstant(3, Privates::GridDimensions.x, 0); // GridWidth

        constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
        cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);
    }


    // Fix velocity at edges
    {

    Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjReflectEdgeVelocity.get());

    cmdList->SetComputeRootDescriptorTable(0, m_dummySRVGPUHandle);
    cmdList->SetComputeRootDescriptorTable(1, m_dummySRVGPUHandle);
    cmdList->SetComputeRootDescriptorTable(2, velocityOutTex->GetUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstant(3, Privates::GridDimensions.x, 0); // GridWidth

    constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
    cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);
    }

    const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        velocityInTex->GetResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        velocityOutTex->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
    cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());

    m_gridVelocityTexPair->Swap();

}

void ComputePassFluidSim2D::FluidStepAdvect(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepAdvect");

    auto bufferInput = m_gridVelocityTexPair->GetInput();
    auto bufferOutput = m_gridVelocityTexPair->GetOutput();

    Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjAdvect.get());

    cmdList->SetComputeRootDescriptorTable(0, bufferInput->GetSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, bufferOutput->GetUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstant(2, Privates::GridDimensions.x, 0); // GridWidth

    constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
    cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);
}

//
//void ComputePassFluidSim2D::FluidStepGrad(ComPtr<ID3D12GraphicsCommandList> cmdList)
//{
//    
//}

void ComputePassFluidSim2D::FluidStepDiffuse(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepDiffuse");

    auto bufferInput = m_gridVelocityTexPair->GetInput();
    auto bufferOutput = m_gridVelocityTexPair->GetOutput();

    Privates::ApplyRootSignatureAndPSO(cmdList, m_computeObjDiffuse.get());

    cmdList->SetComputeRootDescriptorTable(0, bufferInput->GetSRVGPUDescriptorHandle());
    cmdList->SetComputeRootDescriptorTable(1, bufferOutput->GetUAVGPUDescriptorHandle());
    cmdList->SetComputeRoot32BitConstant(2, Privates::GridDimensions.x, 0); // GridWidth

    constexpr ivec2 dispatchSize = Privates::ComputeDispatchSize(Privates::GridDimensions, Privates::ThreadGroupSize);
    cmdList->Dispatch(dispatchSize.x, dispatchSize.y, 1);
}


void ComputePassFluidSim2D::CopySimOutputToDisplayTexture(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "Copy Sim output texture");

    auto bufferOutput = m_gridVelocityTexPair->GetOutput();

    // resource barrier to copy to image render target
    const auto imageRTTransitionToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(m_imageRenderTarget->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    const auto simBufferOutputTransitionToCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(bufferOutput->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    std::vector<CD3DX12_RESOURCE_BARRIER> prepForCopyTransition = { imageRTTransitionToCopyDest , simBufferOutputTransitionToCopySrc };
    cmdList->ResourceBarrier(prepForCopyTransition.size(), prepForCopyTransition.data());

    D3D12_TEXTURE_COPY_LOCATION sourceLoc{
        .pResource = bufferOutput->GetResource(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = 0
    };

    D3D12_TEXTURE_COPY_LOCATION destLoc{
    .pResource = m_imageRenderTarget->GetResource(),
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0
    };
    cmdList->CopyTextureRegion(
        &destLoc,
        0, 0, 0,
        &sourceLoc,
        nullptr);

    //Resource barrier to transition back to original states
    const auto imageRTTransitionToSrv = CD3DX12_RESOURCE_BARRIER::Transition(m_imageRenderTarget->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    const auto simBufferOutputTransitionToUAV = CD3DX12_RESOURCE_BARRIER::Transition(bufferOutput->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    std::vector<CD3DX12_RESOURCE_BARRIER> returnResourcesToOriginalStateTransition = { imageRTTransitionToSrv , simBufferOutputTransitionToUAV };
    cmdList->ResourceBarrier(returnResourcesToOriginalStateTransition.size(), returnResourcesToOriginalStateTransition.data());
}


void ComputePassFluidSim2D::SwapVelocityTexturesStates(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "SwapVelocityTexturesStates");

    // Final swap before restarting the sim loop
    m_gridVelocityTexPair->Swap();
    auto velocityInput = m_gridVelocityTexPair->GetInput();
    auto velocityOutput = m_gridVelocityTexPair->GetOutput();
    const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(velocityInput->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(velocityOutput->GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
    cmdList->ResourceBarrier(buffersStateTransition.size(), buffersStateTransition.data());
}

void ComputePassFluidSim2D::RunSim(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    FluidStepInput(cmdList);
    FluidStepDiv(cmdList);
    FluidStepPressure(cmdList);
    FluidStepProject(cmdList);
    ////Gradient?

    FluidStepAdvect(cmdList);
    //FluidStepDiffuse(cmdList);

    CopySimOutputToDisplayTexture(cmdList);

    SwapVelocityTexturesStates(cmdList);

}

void ComputePassFluidSim2D::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& /*frameResources*/) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "ComputePassFluidSim2D");

    RunSim(cmdList);
}

void ComputePassFluidSim2D::Shutdown()
{
}

//--------------------------------------
namespace GFXPrivates
{
    const std::string MeshName = "Quad";
    void CreateQuadMesh(IRenderer* renderer, MeshLibrary& meshLibrary)
    {
        // Create a quad mesh for rendering the fluid sim

        const float halfSize = 10.0f;
        std::vector<VertexData_Position_UV_POD> verts;
		verts.emplace_back(DirectX::XMFLOAT3(-halfSize, -halfSize, 0), DirectX::XMFLOAT2(0, 0)); // Bottom-left
		verts.emplace_back(DirectX::XMFLOAT3(-halfSize, halfSize, 0), DirectX::XMFLOAT2(0, 1)); // Top-left
		verts.emplace_back(DirectX::XMFLOAT3(+halfSize, halfSize, 0), DirectX::XMFLOAT2(1, 1)); // Top-right
		verts.emplace_back(DirectX::XMFLOAT3(+halfSize, -halfSize, 0), DirectX::XMFLOAT2(1, 0)); // Bottom-right

        const std::vector<std::uint32_t> indices =
        {
            0, 1, 3,
            1, 2, 3,
        };

        meshLibrary.AddMesh(renderer->GetRendererContext(), MeshName, verts, indices);
	}
}


void GraphicsPassFluidSim2D::Init(std::weak_ptr<const ComputePassFluidSim2D> fluidSimComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary)
{
    m_imageSRVIndex = fluidSimComputePass.lock()->GetImageRTSRVIndex();

    m_imageSamplerIndex = AstroTools::Rendering::SamplerIDs::LinearClamp;
    m_imageSamplerGpuHandle = renderer->GetSamplerGPUHandle(m_imageSamplerIndex);

    const auto rootPath = s2ws(DX::GetWorkingDirectory());

    // Create the Chain Element Mesh
    GFXPrivates::CreateQuadMesh(renderer, meshLibrary);

    assert(meshLibrary.GetMesh(GFXPrivates::MeshName, m_quadMesh));
    const auto particleGraphicsShaderPath = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Render.hlsl");

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
            .Num32BitValues = 3
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
    };
    rootSigSlotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

    //Samplers
    D3D12_DESCRIPTOR_RANGE1 samplerRange = {};
    samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    samplerRange.NumDescriptors = (UINT)-1; // Unbounded
    samplerRange.BaseShaderRegister = 0;   // Corresponds to register(s0) in HLSL
    samplerRange.RegisterSpace = 0;
    samplerRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
    samplerRange.OffsetInDescriptorsFromTableStart = 0;
    const D3D12_ROOT_PARAMETER1 rootParamSamplersBindlessResource
    {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
		.DescriptorTable
        {
            .NumDescriptorRanges = 1,
		    .pDescriptorRanges = &samplerRange,
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
    };
    rootSigSlotRootParams.push_back(rootParamSamplersBindlessResource);


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

void GraphicsPassFluidSim2D::Update(int32_t /*frameIdxModulo*/, void* /*Data*/)
{
}

void GraphicsPassFluidSim2D::Execute(
    ComPtr<ID3D12GraphicsCommandList> cmdList,
    float /*deltaTime*/,
    const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "GraphicsPassFluidSim2D");

    const auto indexBuffer = m_quadMesh.lock()->IndexBufferView();
    cmdList->IASetIndexBuffer(&indexBuffer);
    cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pipelineStateObject.Get());

    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);

    const uint32_t GraphicsBindlessResourceIndicesRootSigParamIndex = 1;
    const std::vector<int32_t> GraphicsBindlessResourceIndices = {
        m_imageSRVIndex,
        m_imageSamplerIndex,
        m_quadMesh.lock()->GetVertexBufferSRV()
    };
    cmdList->SetGraphicsRoot32BitConstants(
        (UINT)GraphicsBindlessResourceIndicesRootSigParamIndex,
        (UINT)GraphicsBindlessResourceIndices.size(), GraphicsBindlessResourceIndices.data(), 0);

    cmdList->SetGraphicsRootDescriptorTable(
        2,
        m_imageSamplerGpuHandle
    );

    cmdList->DrawIndexedInstanced((UINT)m_quadMesh.lock()->GetVertexIndicesCount(), 1, 0, 0, 0);
}

void GraphicsPassFluidSim2D::Shutdown()
{

}

