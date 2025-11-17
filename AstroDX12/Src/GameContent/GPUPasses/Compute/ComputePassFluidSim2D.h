#pragma once

#include <memory>

#include <Rendering/Common/GPUPass.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/MeshLibrary.h>

#include <Rendering/Compute/ComputableObject.h>
#include <Rendering/Common/RenderTarget.h>

class ComputePassFluidSim2D :
    public ComputePass
{
public:

    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;


private:
    
    int32_t m_frameIdxModulo = 0;
    //SimNeedsResetData m_simNeedsReset;

    void FluidStepInput(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepAdvect(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepDiv(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    //void FluidStepGrad(ComPtr<ID3D12GraphicsCommandList> cmdList);
    void FluidStepDiffuse(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepPressure(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepProject(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void RunSim(ComPtr<ID3D12GraphicsCommandList> cmdList) const;


    std::shared_ptr<RenderTarget> m_gridPing;
    std::shared_ptr<RenderTarget> m_gridPong;

    std::unique_ptr<ComputableObject> m_computeObjInput;
    std::unique_ptr<ComputableObject> m_computeObjAdvect;
    std::unique_ptr<ComputableObject> m_computeObjDiffuse;
    std::unique_ptr<ComputableObject> m_computeObjDivergence;
    std::unique_ptr<ComputableObject> m_computeObjPressure;
    std::unique_ptr<ComputableObject> m_computeObjProject;
    //std::unique_ptr<ComputableObject> m_computeObjGradient;

};

// Graphics
//ComPtr<ID3D12PipelineState> m_pipelineStateObject;
//ComPtr<ID3D12RootSignature> m_rootSignature;
