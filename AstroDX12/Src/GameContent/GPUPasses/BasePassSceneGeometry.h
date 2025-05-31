#pragma once

#include <map>
#include <vector>
#include <Common.h>

#include <Rendering/Common/GPUPass.h>
#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/Common/StructuredBuffer.h>

class RenderableGroup;
class IRenderer;
using Microsoft::WRL::ComPtr;

using RootSignaturePSOPair = std::pair< const ComPtr< ID3D12RootSignature>, const ComPtr<ID3D12PipelineState>>;
using RenderableGroupMap = std::map<RootSignaturePSOPair, std::unique_ptr<RenderableGroup>>;

class BasePassSceneGeometry : public GraphicsPass
{

public:
    BasePassSceneGeometry(/*const std::string& name*/)
		: GraphicsPass(/*name*/) 
        , m_frameIdxModulo(0)
        , m_renderableObjectConstantsDataBufferPerFrameResources()
        , m_renderableGroupMap()
    {
	
    }

    void Init(IRenderer* renderer, const std::vector<IRenderableDesc>& renderablesDesc, int16_t numFrameResources);
    virtual void Update(int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    int32_t m_frameIdxModulo;
    std::vector<std::unique_ptr<StructuredBuffer<RenderableObjectConstantData>>> m_renderableObjectConstantsDataBufferPerFrameResources;

    RenderableGroupMap m_renderableGroupMap;
};

