#pragma once
#include "IRenderable.h"
#include "RenderData/Mesh.h"
#include "../Common.h"
#include "../Maths/MathUtils.h"

struct RenderableObjectConstantData
{
public:
	DirectX::XMFLOAT4X4 WorldViewProj = AstroTools::Maths::Identity4x4();
};

class RenderableStaticObject : public IRenderable
{
public:
	explicit RenderableStaticObject(
		std::unique_ptr<Mesh>& mesh,
		ComPtr<ID3D12RootSignature>& rootSignature,
		std::unique_ptr<UploadBuffer<RenderableObjectConstantData>>& constantBuffer,
		ComPtr<ID3D12PipelineState>& pipelineStateObject
	)
		: m_mesh(std::move(mesh))
		, m_rootSignature( rootSignature )
		, m_constantBuffer(std::move(constantBuffer))
		, m_pipelineStateObject(pipelineStateObject)
	{
	}

	virtual ComPtr<ID3D12RootSignature> GetGraphicsRootSignature() const override
	{
		return m_rootSignature;
	}
	virtual D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const override
	{
		return m_mesh->VertexBufferView();
	}
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const override
	{
		return m_mesh->IndexBufferView();
	}
	virtual UINT GetIndexCount() const override { return m_mesh->IndexCount;  }

	virtual void SetConstantBufferData(const void* data) override
	{
		assert(data);
		auto typedData = static_cast<const RenderableObjectConstantData*>(data);
		assert(typedData);
		m_constantBuffer->CopyData(0, *typedData);
	}

	virtual ComPtr<ID3D12PipelineState> GetPipelineStateObject() const override
	{
		return m_pipelineStateObject;
	}

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
	std::unique_ptr<Mesh> m_mesh;
	std::unique_ptr<UploadBuffer<RenderableObjectConstantData>> m_constantBuffer;
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;

};