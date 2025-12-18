#pragma once

#include <memory>
#include <Rendering/Common/GPUPass.h>
#include <Rendering/Compute/ComputableObject.h>
#include <directxmath.h>
#include <Rendering/Common/StructuredBuffer.h>
#include <Rendering/RenderData/Mesh.h>

using Microsoft::WRL::ComPtr;

class IRenderer;
class MeshLibrary;
namespace AstroTools::Rendering
{
    class ShaderLibrary;
}

struct ParticleData
{
    ParticleData()
        : Pos(0.f, 0.f, 0.f)
        , Vel(0.f,0.f,0.f)
        , Age(0.f)
        , Duration(5.f)
		, Size(1.f)
    {
    }
    
    // Make sure to keep Input layout in sync
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Vel;
    float Age;
    float Duration;
    float Size;
};



class ComputePassParticles :
    public ComputePass
{
public:
    ComputePassParticles();

    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
    virtual void Update(const GPUPassUpdateData& updateData) override;
    virtual void Execute(
        ComPtr<ID3D12GraphicsCommandList> cmdList,
        float deltaTime,
        const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

    int32_t GetParticleReadBufferSRVHeapIndex() const;
    int32_t GetParticleOutputBufferSRVHeapIndex() const;

private:
    int32_t m_frameIdxModulo = 0;

    std::unique_ptr<StructuredBuffer<ParticleData>> m_particleDataBufferPing;
    std::unique_ptr<StructuredBuffer<ParticleData>> m_particleDataBufferPong;

    std::unique_ptr<ComputableObject> m_particlesComputeObj;
};

class GraphicsPassParticles : public GraphicsPass
{
public:
    GraphicsPassParticles();
    void Init(std::weak_ptr<const ComputePassParticles> particlesComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, const MeshLibrary& meshLibrary);
    virtual void Update(const GPUPassUpdateData& updateData) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    std::weak_ptr<IMesh> m_boxMesh;
    ComPtr<ID3D12PipelineState> m_pipelineStateObject;
    ComPtr<ID3D12RootSignature> m_rootSignature;

    std::weak_ptr<const ComputePassParticles> m_particlesComputePass;
};