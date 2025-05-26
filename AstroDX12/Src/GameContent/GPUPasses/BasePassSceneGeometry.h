#pragma once

#include <map>
#include <vector>
#include <Common.h>

#include <Rendering/Common/GPUPass.h>
#include <Rendering/Renderable/IRenderable.h>

class RenderableGroup;
using Microsoft::WRL::ComPtr;

using RootSignaturePSOPair = std::pair< const ComPtr< ID3D12RootSignature>, const ComPtr<ID3D12PipelineState>>;
using RenderableGroupMap = std::map<RootSignaturePSOPair, std::unique_ptr<RenderableGroup>>;

class BasePassSceneGeometry : public GraphicsPass
{

public:
    BasePassSceneGeometry(/*const std::string& name*/)
		: GraphicsPass(/*name*/) 
    {
	
    }

    void Init(const std::vector<IRenderableDesc>& renderableObjects, int16_t numFrameResources);
    virtual void Update(void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    RenderableGroupMap m_renderableGroupMap;
};

