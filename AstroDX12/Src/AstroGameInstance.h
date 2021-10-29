#pragma once

#include "Game.h"
#include "Common.h"
#include "Rendering/Common/UploadBuffer.h"
#include "Rendering/RenderableStaticObject.h"
#include "Rendering/RenderData/Mesh.h"
#include "Maths/MathUtils.h"

using Microsoft::WRL::ComPtr;
class IRenderableDesc;

class AstroGameInstance final :
    public Game 
{
public:
    virtual void LoadSceneData() override;
    // Scene renderable objects building
    virtual void BuildConstantBuffers() override;
    virtual void BuildRootSignature() override;
    virtual void BuildShadersAndInputLayout() override;
    virtual void BuildSceneGeometry() override;
    virtual void BuildPipelineStateObject() override;

private:
    ComPtr<ID3DBlob> m_vertexShaderByteCode = nullptr;
    ComPtr<ID3DBlob> m_pixelShaderByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
    ComPtr<ID3D12PipelineState> m_pipelineStateObject = nullptr;

    std::vector<IRenderableDesc> m_renderablesDesc;

    XMFLOAT4X4 m_worldMat = AstroTools::Maths::Identity4x4();
    XMFLOAT4X4 m_viewMat = AstroTools::Maths::Identity4x4();
    XMFLOAT4X4 m_projMat = AstroTools::Maths::Identity4x4();

    const float m_theta = 1.5f * XM_PI;
    const float m_phi = XM_PIDIV4;
    const float m_radius = 5.0f;

    virtual void CreateRenderables() override;
    virtual void Update(float deltaTime) override;
    virtual void Render(float deltaTime) override;
};

