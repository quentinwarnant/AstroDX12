#pragma once

#include <Rendering/Common/GPUPass.h>
#include <Common.h>
#include <Rendering/Common/RendererContext.h>
#include <Rendering/Common/StructuredBuffer.h>
#include <Rendering/Common/ShaderLibrary.h>


class IRenderer;
struct FrameResource;
class IMesh;
class MeshLibrary;
using Microsoft::WRL::ComPtr;

class GraphicsPassDebugDraw : public GraphicsPass
{
public:
	GraphicsPassDebugDraw()
		: GraphicsPass()
	{
	}
	void Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary);
	virtual void Update(const GPUPassUpdateData& updateData) override;
	virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
	virtual void Shutdown() override;

	int32_t GetDebugObjectsBufferUAVIndex() const
	{
		return m_debugObjectsBuffer->GetUAVIndex();
	}

private:
	
	void CreateRootSignature(IRenderer* renderer);
	void CreatePipelineState(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary);

	struct DebugObjectData
	{
		DirectX::XMFLOAT4X4 Transform;
		DirectX::XMFLOAT3 Color;
	};
	std::unique_ptr<StructuredBuffer<DebugObjectData>> m_debugObjectsBuffer;

	ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
	ComPtr<ID3D12PipelineState> m_pso = nullptr;

	std::weak_ptr<IMesh> m_debugMesh;

};

