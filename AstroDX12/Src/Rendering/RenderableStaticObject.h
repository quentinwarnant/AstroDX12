#pragma once
#include "IRenderable.h"
#include "RenderData/Mesh.h"
#include "../Common.h"

class RenderableStaticObject : public IRenderable
{
public:
	explicit RenderableStaticObject(Mesh& mesh, ComPtr<ID3D12RootSignature>& rootSignature)
		: m_mesh(std::make_unique<Mesh>(mesh))
		, m_rootSignature( rootSignature )
	{}

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

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
	std::unique_ptr<Mesh> m_mesh;
};