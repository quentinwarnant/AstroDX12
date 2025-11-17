#include "ComputePassFluidSim2D.h"

namespace Privates
{
    constexpr DirectX::XMUINT2 GridDimensions = DirectX::XMUINT2(256, 256);

    std::unique_ptr<ComputableObject> CreateComputableObject(
        IRenderer* renderer,
        AstroTools::Rendering::ShaderLibrary& shaderLibrary,
        const std::wstring& computeShaderPath)
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
        ranges.push_back(srvRange);

        // UAV descriptor table for RWTexture2D (BufferOut)
        D3D12_DESCRIPTOR_RANGE1 uavRange = {};
        uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uavRange.NumDescriptors = 1;
        uavRange.BaseShaderRegister = 0; // u0
        uavRange.RegisterSpace = 0;
        uavRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
        uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        ranges.push_back(uavRange);

        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

        // SRV table
        D3D12_ROOT_PARAMETER1 srvTable = {};
        srvTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        srvTable.DescriptorTable.NumDescriptorRanges = 1;
        srvTable.DescriptorTable.pDescriptorRanges = &ranges[0];
        srvTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        slotRootParams.push_back(srvTable);

        // UAV table
        D3D12_ROOT_PARAMETER1 uavTable = {};
        uavTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        uavTable.DescriptorTable.NumDescriptorRanges = 1;
        uavTable.DescriptorTable.pDescriptorRanges = &ranges[1];
        uavTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        slotRootParams.push_back(uavTable);

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
        computableObjDesc.CS = shaderLibrary.GetCompiledShader(computableObjDesc.ComputeShaderPath, L"CSMain", {}, L"cs_6_6");

        // Compile PSO
        renderer->CreateComputePipelineState(
            computableObjDesc.PipelineStateObject,
            computableObjDesc.RootSignature,
            computableObjDesc.CS);

        return std::make_unique<ComputableObject>(computableObjDesc.RootSignature, computableObjDesc.PipelineStateObject);
    }


    void ComputePassCommon(ComPtr<ID3D12GraphicsCommandList> cmdList, ComputableObject* computableObj, RenderTarget* bufferInput, RenderTarget* bufferOutput)
    {
        cmdList->SetComputeRootSignature(computableObj->GetRootSignature().Get());
        cmdList->SetPipelineState(computableObj->GetPSO().Get());

		cmdList->SetComputeRootDescriptorTable(0, bufferInput->GetSRVGPUDescriptorHandle());
		cmdList->SetComputeRootDescriptorTable(1, bufferOutput->GetUAVGPUDescriptorHandle());

        constexpr int32_t dispatchX = (Privates::GridDimensions.x + 31) / 32;
        constexpr int32_t dispatchY = (Privates::GridDimensions.y + 31) / 32;
        cmdList->Dispatch(dispatchX, dispatchY, 1);
    }
}

void ComputePassFluidSim2D::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    m_gridPing = std::make_unique<RenderTarget>();
    m_gridPong = std::make_unique<RenderTarget>();

    //First input is Pong, and output is ping
    renderer->InitialiseRenderTarget(m_gridPing, L"FluidSim2D::Grid::Ping", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32G32_FLOAT, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    renderer->InitialiseRenderTarget(m_gridPong, L"FluidSim2D::Grid::Pong", Privates::GridDimensions.x, Privates::GridDimensions.y, DXGI_FORMAT_R32G32_FLOAT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    const auto computeShaderPathInput = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Input.hlsl");
    m_computeObjInput = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathInput);

    const auto computeShaderPathAdvect = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Advect.hlsl");
    m_computeObjAdvect = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathAdvect);

    const auto computeShaderPathDivergence = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Div.hlsl");
    m_computeObjDivergence = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathDivergence);

    //const auto computeShaderPathGradient = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Grad.hlsl");
    //m_computeObjGradient = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathGradient);

    const auto computeShaderPathDiffuse = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Diffuse.hlsl");
    m_computeObjDiffuse = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathDiffuse);

    const auto computeShaderPathPressure = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Pressure.hlsl");
    m_computeObjPressure = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathPressure);

    const auto computeShaderPathProject = rootPath + std::wstring(L"\\Shaders\\FluidSim\\Project.hlsl");
    m_computeObjProject = Privates::CreateComputableObject(renderer, shaderLibrary, computeShaderPathProject);
}

void ComputePassFluidSim2D::Update(int32_t frameIdxModulo, void* /*Data*/)
{
    m_frameIdxModulo = frameIdxModulo;

    //m_simNeedsReset.Tick();
}

void ComputePassFluidSim2D::FluidStepInput(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepInput");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_gridPing.get() : m_gridPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_gridPing.get() : m_gridPong.get();

    // Transition resources into their next correct state
    const auto bufferInStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferInput->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    const auto bufferOutStateTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        bufferOutput->GetResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransition = { bufferInStateTransition , bufferOutStateTransition };
    cmdList->ResourceBarrier(2, buffersStateTransition.data());

    Privates::ComputePassCommon(cmdList, m_computeObjInput.get(), bufferInput, bufferOutput);
}

void ComputePassFluidSim2D::FluidStepDiv(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepDiv");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 1 ? m_gridPing.get() : m_gridPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 0 ? m_gridPing.get() : m_gridPong.get();

    Privates::ComputePassCommon(cmdList, m_computeObjDivergence.get(), bufferInput, bufferOutput);
}

void ComputePassFluidSim2D::FluidStepPressure(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepPressure");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_gridPing.get() : m_gridPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_gridPing.get() : m_gridPong.get();

	constexpr int32_t pressureIterations = 60;
    for (int32_t i = 0; i < pressureIterations; ++i)
    {
        Privates::ComputePassCommon(cmdList, m_computeObjPressure.get(), bufferInput, bufferOutput);
        // Swap input and output for next iteration
        std::swap(bufferInput, bufferOutput);
	}
}

void ComputePassFluidSim2D::FluidStepProject(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepProject");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_gridPing.get() : m_gridPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_gridPing.get() : m_gridPong.get();

    Privates::ComputePassCommon(cmdList, m_computeObjProject.get(), bufferInput, bufferOutput);
}

void ComputePassFluidSim2D::FluidStepAdvect(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepAdvect");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 1 ? m_gridPing.get() : m_gridPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 0 ? m_gridPing.get() : m_gridPong.get();

    Privates::ComputePassCommon(cmdList, m_computeObjAdvect.get(), bufferInput, bufferOutput);
}


//
//void ComputePassFluidSim2D::FluidStepGrad(ComPtr<ID3D12GraphicsCommandList> cmdList)
//{
//    
//}

void ComputePassFluidSim2D::FluidStepDiffuse(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "FluidStepDiffuse");

    // Dispatch Particle update
    auto bufferInput = m_frameIdxModulo % 2 == 0 ? m_gridPing.get() : m_gridPong.get();
    auto bufferOutput = m_frameIdxModulo % 2 == 1 ? m_gridPing.get() : m_gridPong.get();

    Privates::ComputePassCommon(cmdList, m_computeObjDiffuse.get(), bufferInput, bufferOutput);
}


void ComputePassFluidSim2D::RunSim(ComPtr<ID3D12GraphicsCommandList> cmdList) const
{
    FluidStepInput(cmdList);
    FluidStepDiv(cmdList);
    FluidStepPressure(cmdList);
    FluidStepProject(cmdList);
    //Gradient?
    FluidStepAdvect(cmdList);
    FluidStepDiffuse(cmdList);
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
    m_gridPing->ReleaseResources();
    m_gridPong->ReleaseResources();

    //m_computeObjInput.release();
}