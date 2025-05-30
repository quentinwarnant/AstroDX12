#pragma once

#include <memory>
#include <Rendering/Common/GPUPass.h>
#include <Rendering/Compute/ComputableObject.h>
#include <directxmath.h>
#include <Rendering/Common/StructuredBuffer.h>

using Microsoft::WRL::ComPtr;

class IRenderer;
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
    {
    }
    
    // Make sure to keep Input layout in sync
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Vel;
    float Age;
    float Duration;
    // Or add padding!
};


class ComputePassParticles :
    public ComputePass
{
public:
    ComputePassParticles();

    void Init(IRenderer& renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
    virtual void Update(void* Data) override;
    virtual void Execute(
        ComPtr<ID3D12GraphicsCommandList> cmdList,
        float deltaTime,
        const FrameResource& frameResources,
        const DescriptorHeap& descriptorHeap,
        int32_t descriptorSizeCBV) const override;
    virtual void Shutdown() override;

private:
    int32_t m_frameIdx = 0;

    std::unique_ptr<StructuredBuffer<ParticleData>> m_particleDataBufferPing;
    std::unique_ptr<StructuredBuffer<ParticleData>> m_particleDataBufferPong;

    std::unique_ptr<ComputableObject> m_particlesComputeObj;
};

