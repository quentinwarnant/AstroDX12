#pragma once

#include <Rendering/Common/GPUPass.h>
#include <Rendering/Compute/ComputeGroup.h>

using Microsoft::WRL::ComPtr;

class ComputePassAddBufferValues :
    public ComputePass
{
public:
    ComputePassAddBufferValues()
        : m_computeGroup(std::make_unique<ComputeGroup>())
    {
    }

    void Init(const std::vector<ComputableDesc>& computableDescs);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(
        ComPtr<ID3D12GraphicsCommandList> cmdList,
        float deltaTime,
        const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    std::unique_ptr<ComputeGroup> m_computeGroup;
};