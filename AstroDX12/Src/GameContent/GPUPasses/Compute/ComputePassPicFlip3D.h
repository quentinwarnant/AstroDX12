#pragma once


#include <Rendering/Common/GPUPass.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/Compute/ComputableObject.h>

namespace PicFlip
{
    struct ParticleData
    {
        ParticleData()
            : Pos(0.f, 0.f, 0.f)
            , Vel(0.f, 0.f, 0.f)
        {
        }

        // Make sure to keep Input layout in sync
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Vel;
    };
}


class ComputePassPicFlip3D :
    public ComputePass
{
public:
    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
    virtual void Update(const GPUPassUpdateData& updateData) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

    int32_t GetParticleReadBufferSRVHeapIndex() const;
    int32_t GetParticleOutputBufferSRVHeapIndex() const;

private:
	int32_t m_frameIdxModulo = 0;
	int32_t m_ParticleReadBufferSRVIndex = 0;

    std::unique_ptr<StructuredBuffer<PicFlip::ParticleData>> m_particleDataBufferPing;
    std::unique_ptr<StructuredBuffer<PicFlip::ParticleData>> m_particleDataBufferPong;

    std::unique_ptr<ComputableObject> m_particlesComputeObj;

};


// Graphics
class GraphicsPassPicFlip3D : public GraphicsPass
{
public:
    void Init(std::weak_ptr<const ComputePassPicFlip3D> fluidSimComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary);
    virtual void Update(const GPUPassUpdateData& updateData) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    int32_t m_sphereMeshSRVIndex;

    std::weak_ptr<const ComputePassPicFlip3D> m_fluidSimComputePass;
    std::weak_ptr<IMesh> m_sphereMesh;
    ComPtr<ID3D12PipelineState> m_pipelineStateObject;
    ComPtr<ID3D12RootSignature> m_rootSignature;


};