#pragma once

#include <Game.h>
#include <Common.h>
#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/Common/UploadBuffer.h>
#include <Rendering/Common/FrameResource.h>
#include <Rendering/Renderable/RenderableStaticObject.h>
#include <Rendering/RenderData/Mesh.h>
#include <Rendering/Compute/IComputable.h>
#include <Maths/MathUtils.h>

using Microsoft::WRL::ComPtr;
class IRenderableDesc;
struct SceneData;

namespace AstroTools::Rendering
{
    class ShaderLibrary;
}

class AstroGameInstance final :
    public Game 
{
public:
    AstroGameInstance();
    virtual void LoadSceneData() override;
    // Scene renderable objects building
    virtual void BuildFrameResources() override;
    virtual void Create_const_uav_srv_BufferDescriptorHeaps() override;
    virtual void CreateConstantBufferViews() override;
    virtual void CreateComputableObjectsStructuredBufferViews() override;
    virtual void BuildRootSignature() override;
    virtual void BuildShaders(AstroTools::Rendering::ShaderLibrary& shaderLibrary) override;
    virtual void BuildPipelineStateObject() override;

private:
    void BuildSceneGeometry();
    void UpdateFrameResource();
    void UpdateRenderablesConstantBuffers();
    void UpdateMainRenderPassConstantBuffer(float deltaTime);

    void BuildComputeData();

    SceneData LoadSceneGeometry();

    std::vector<IRenderableDesc> m_renderablesDesc;
    std::vector<ComputableDesc> m_computableDescs;

    XMFLOAT3 m_cameraPos;

    XMFLOAT4X4 m_viewMat = AstroTools::Maths::Identity4x4();
    XMFLOAT4X4 m_projMat = AstroTools::Maths::Identity4x4();

    const float m_theta = 1.5f * XM_PI;
    const float m_phi = XM_PIDIV4;
    const float m_radius = 5.0f;

    std::unique_ptr<MeshLibrary> m_meshLibrary;

    // Resources
    std::vector<std::unique_ptr<FrameResource>> m_frameResources;
    FrameResource* m_currentFrameResource = nullptr;
    int m_currentFrameResourceIndex = 0;

    virtual void CreateRenderables() override;
    virtual void CreateComputables() override;
    virtual void Update(float deltaTime) override;
    virtual void Render(float deltaTime) override;
};

