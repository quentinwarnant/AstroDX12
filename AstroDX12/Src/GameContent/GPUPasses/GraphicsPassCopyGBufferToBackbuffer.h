#pragma once

#include <Rendering/Common/GPUPass.h>
#include <Rendering/Common/ShaderLibrary.h>

class IRenderer;
using Microsoft::WRL::ComPtr;

class GraphicsPassCopyGBufferToBackbuffer : public GraphicsPass
{
public:
    GraphicsPassCopyGBufferToBackbuffer()
		: m_indexBufferGPU(nullptr)
		, m_indexUploadBuffer(nullptr)
		, m_indexBufferView{}
		, m_rootSignature(nullptr)
		, m_pso(nullptr)
		, m_GBufferRTViewIndex(-1)
    {
    }

    void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, int32_t GBufferRTViewIndex);
    virtual void Update(float deltaTime, int32_t frameIdxModulo, void* Data) override;
    virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
    virtual void Shutdown() override;

private:
    ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;
    ComPtr<ID3D12Resource> m_indexUploadBuffer = nullptr;

    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
    ComPtr<ID3D12PipelineState> m_pso = nullptr;

	int32_t m_GBufferRTViewIndex = -1;
};

