#pragma once

#include <Rendering/Common/GPUPass.h>
#include <Rendering/Compute/ComputeGroup.h>
#include <Rendering/Common/RenderTarget.h>
#include <Rendering/Common/StructuredBuffer.h>
#include <Rendering/Common/ShaderLibrary.h>

#include <GameContent/GPUPasses/Compute/ComputePassParticles.h>

using Microsoft::WRL::ComPtr;

struct SDFSceneObject
{
    SDFSceneObject()
        : Pos(0.f, 0.f, 0.f, 0.f)
    {
    }

    // Make sure to keep Input layout in sync
    DirectX::XMFLOAT4 Pos;
};


class ComputePassRaymarchScene :
    public ComputePass
{
public:
    ComputePassRaymarchScene()
        : m_gBuffer1RT(nullptr)
        , m_raymarchRootSignature(nullptr)
		, m_raymarchPSO(nullptr)
		, m_currentParticleDataBufferSRVIdx(-1)
    {
    }

    struct UpdateContext
    {
		ComputePassParticles* ParticleComputePass = nullptr;
    };

    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, std::weak_ptr<ComputePassParticles> particleComputePass);
    virtual void Update(const GPUPassUpdateData& updateData) override;
    virtual void Execute(
        ComPtr<ID3D12GraphicsCommandList> cmdList,
        float deltaTime,
        const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

    int32_t GetGBufferRTViewIndex() const
    {
        return m_gBuffer1RT->GetSRVIndex();
	}

private:
	std::shared_ptr<RenderTarget> m_gBuffer1RT;
    std::unique_ptr<StructuredBuffer<SDFSceneObject>> m_SDFSceneObjectsBuffer;

    ComPtr<ID3D12RootSignature> m_raymarchRootSignature;
    ComPtr<ID3D12PipelineState> m_raymarchPSO;

    std::weak_ptr<ComputePassParticles> m_particleComputePass;
    int32_t m_currentParticleDataBufferSRVIdx = -1;


};

