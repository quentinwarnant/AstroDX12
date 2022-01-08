#pragma once

#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/RenderData/Mesh.h>
#include <Common.h>
#include <Maths/MathUtils.h>
#include <Rendering/RenderData/RenderConstants.h>

using namespace DirectX;

class RenderableStaticObject : public IRenderable
{
public:
	explicit RenderableStaticObject(
		std::weak_ptr<Mesh>& mesh,
		ComPtr<ID3D12RootSignature>& rootSignature,
		ComPtr<ID3D12PipelineState>& pipelineStateObject, 
		XMFLOAT4X4& initialTransform,
		int16_t objectIndex
	)
		: m_transform(initialTransform)
		, m_mesh(mesh)
		, m_rootSignature( rootSignature )
		, m_pipelineStateObject(pipelineStateObject)
		, m_dirtyFrameCount(0)
		, m_objectsConstantBufferIndex(objectIndex)
	{
	}

	//virtual void SetPosition(const XMVECTOR& pos) override
	//{
	//}

	//virtual void SetScale(const XMVECTOR& pos) override
	//{
	//}
	//
	//virtual void SetRotation(const XMVECTOR& pos) override
	//{
	//}

	virtual const XMFLOAT4X4& GetWorldTransform() const override
	{
		return m_transform;
	}

	virtual ComPtr<ID3D12RootSignature> GetGraphicsRootSignature() const override
	{
		return m_rootSignature;
	}
	virtual D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const override
	{
		return m_mesh.lock()->VertexBufferView();
	}
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const override
	{
		return m_mesh.lock()->IndexBufferView();
	}
	virtual UINT GetIndexCount() const override { return m_mesh.lock()->IndexCount;  }

	virtual const ComPtr<ID3D12PipelineState>& GetPipelineStateObject() const override
	{
		return m_pipelineStateObject;
	}

	virtual bool IsDirty() const override
	{
		return m_dirtyFrameCount > 0;
	}

	virtual void MarkDirty(int16_t dirtyFrameCount) override
	{
		m_dirtyFrameCount = dirtyFrameCount;
	}

	virtual void ReduceDirtyFrameCount() override
	{
		assert(m_dirtyFrameCount > 0);
		m_dirtyFrameCount--;
	}

	virtual int16_t GetConstantBufferIndex() const override
	{
		return m_objectsConstantBufferIndex;
	}

private:
	XMFLOAT4X4 m_transform;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	std::weak_ptr<Mesh> m_mesh;
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;

	// Amount of frame resources remaining that need to be updated to newer data
	int16_t m_dirtyFrameCount;

	// Index into which object constant buffer this object corresponds to
	int16_t m_objectsConstantBufferIndex;
};