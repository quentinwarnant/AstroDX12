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

namespace PhysicsChain
{

    struct ParticleData
    {
        ParticleData()
            : Pos(0.f, 0.f, 0.f)
        {
        }

        // Make sure to keep Input layout in sync
        DirectX::XMFLOAT3 Pos;
    };


    struct ChainElementData
    {
	    ParticleData Particle;
	    int32_t ParentIndex;
	    float RestLength;
        bool Pinned;
    };

}


class ComputePassPhysicsChain :
    public ComputePass
{
public:
    ComputePassPhysicsChain();

    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(
        ComPtr<ID3D12GraphicsCommandList> cmdList,
        float deltaTime,
        const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

    int32_t GetParticleReadBufferSRVHeapIndex() const;
    int32_t GetParticleOutputBufferSRVHeapIndex() const;

private:
    int32_t m_frameIdxModulo = 0;

    std::unique_ptr<StructuredBuffer<PhysicsChain::ChainElementData>> m_chainDataBufferPing;
    std::unique_ptr<StructuredBuffer<PhysicsChain::ChainElementData>> m_chainDataBufferPong;

    std::unique_ptr<ComputableObject> m_particlesComputeObj;
};

class GraphicsPassPhysicsChain : public GraphicsPass
{
public:
    GraphicsPassPhysicsChain();
    void Init(std::weak_ptr<const ComputePassPhysicsChain> particlesComputePass, IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    void CreateChainElementMesh(IRenderer* renderer, MeshLibrary& meshLibrary);
    std::weak_ptr<IMesh> m_chainElementMesh;
    ComPtr<ID3D12PipelineState> m_pipelineStateObject;
    ComPtr<ID3D12RootSignature> m_rootSignature;

    std::weak_ptr<const ComputePassPhysicsChain> m_particlesComputePass;
};