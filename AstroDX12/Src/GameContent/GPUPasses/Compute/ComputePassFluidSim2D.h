#pragma once

#include <memory>

#include <Rendering/Common/GPUPass.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/MeshLibrary.h>

#include <Rendering/Compute/ComputableObject.h>
#include <Rendering/Common/RenderTarget.h>
#include <Rendering/Common/RenderTargetPair.h>

class ComputePassFluidSim2D :
    public ComputePass
{
public:

    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

    int32_t GetImageRTSRVIndex() const
    {
        return m_imageRenderTarget->GetSRVIndex();
	}

private:
    
    int32_t m_frameIdxModulo = 0;
    //SimNeedsResetData m_simNeedsReset;

    void RunSim(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepInput(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepAdvect(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepDiv(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    //void FluidStepGrad(ComPtr<ID3D12GraphicsCommandList> cmdList);
    void FluidStepDiffuse(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepPressure(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void FluidStepProject(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void CopySimOutputToDisplayTexture(ComPtr<ID3D12GraphicsCommandList> cmdList) const;
    void SwapVelocityTexturesStates(ComPtr<ID3D12GraphicsCommandList> cmdList) const;

    std::unique_ptr<RenderTargetPair> m_gridVelocityTexPair;
    std::unique_ptr<RenderTarget> m_gridDivergenceTex;
    std::unique_ptr<RenderTargetPair> m_gridPressureTexPair;
	std::unique_ptr<RenderTarget> m_imageRenderTarget;

    std::unique_ptr<ComputableObject> m_computeObjInput;
    std::unique_ptr<ComputableObject> m_computeObjAdvect;
    std::unique_ptr<ComputableObject> m_computeObjDiffuse;
    std::unique_ptr<ComputableObject> m_computeObjDivergence;
    std::unique_ptr<ComputableObject> m_computeObjPressure;
    std::unique_ptr<ComputableObject> m_computeObjProject;
    //std::unique_ptr<ComputableObject> m_computeObjGradient;

};

// Graphics
class GraphicsPassFluidSim2D : public GraphicsPass
{
public: 
    void Init(std::weak_ptr<const ComputePassFluidSim2D> fluidSimComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    int32_t m_imageSRVIndex;
    int32_t m_imageSamplerIndex;
    D3D12_GPU_DESCRIPTOR_HANDLE m_imageSamplerGpuHandle;

    std::weak_ptr<IMesh> m_quadMesh;
    ComPtr<ID3D12PipelineState> m_pipelineStateObject;
    ComPtr<ID3D12RootSignature> m_rootSignature;


};
//ComPtr<ID3D12PipelineState> m_pipelineStateObject;
//ComPtr<ID3D12RootSignature> m_rootSignature;
