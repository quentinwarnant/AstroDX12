#pragma once
#include <Rendering/Common/GPUPass.h>
#include <Rendering/Common/StructuredBuffer.h>

class IRenderer;

namespace AstroTools::Rendering
{
	class ShaderLibrary;
}
//class AstroTools::Rendering::ShaderLibrary;

class ComputePassVertexLineDebugDraw : public ComputePass
{
public:
	ComputePassVertexLineDebugDraw()
		: ComputePass()
	{
	}
	void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);
	virtual void Update(const GPUPassUpdateData& updateData) override;
	virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
	virtual void Shutdown() override;

	int32_t GetDebugDrawLineVertexBufferUAVIndex() const
	{
		return m_debugDataBuffer->GetUAVIndex();
	}

	int32_t GetLineCountBufferUAVIndex() const
	{
		return m_lineCountBuffer->GetUAVIndex();
	}

private: 
	struct DrawIndirectArgs
	{
		uint32_t DebugDrawLineBufferIdx;
		uint32_t GlobalCBVIdx;
		D3D12_DRAW_ARGUMENTS DrawArgs;
	};

	void CreateRootSignatures(IRenderer* renderer);
	void CreatePipelineState(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);

	void ExecuteComputeIndirectArgs(ComPtr<ID3D12GraphicsCommandList> cmdList, const FrameResource& frameResources) const;
	void ExecuteIndirectDraw(ComPtr<ID3D12GraphicsCommandList> cmdList, const FrameResource& frameResources) const;


	struct DebugVertexLineData
	{
		DirectX::XMFLOAT3 Pos;
		std::int32_t ColorIndex;
	};

	std::unique_ptr<StructuredBuffer<DebugVertexLineData>> m_debugDataBuffer;
	std::unique_ptr<StructuredBuffer<DrawIndirectArgs>> m_indirectArgsBuffer;
	std::unique_ptr<StructuredBuffer<uint32_t>> m_lineCountBuffer;

	ComPtr<ID3D12RootSignature> m_rsComputeIndirectArgs = nullptr;
	ComPtr<ID3D12PipelineState> m_psoComputeIndirectArgs = nullptr;

	ComPtr<ID3D12CommandSignature> m_rsDispatchIndirectDraw = nullptr;

	ComPtr<ID3D12RootSignature> m_rsDrawDebug = nullptr;
	ComPtr<ID3D12PipelineState> m_psoDrawDebug = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE m_lineCountBufferGPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_lineCountBufferCPUHandle;
};

