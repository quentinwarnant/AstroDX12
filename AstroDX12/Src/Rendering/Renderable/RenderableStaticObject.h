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
		const IRenderableDesc& InRenderableDesc,
		int16_t objectIndex
	)
		: m_transform( InRenderableDesc.InitialTransform)
		, m_mesh(InRenderableDesc.Mesh)
		, m_rootSignature(InRenderableDesc.RootSignature)
		, m_pipelineStateObject(InRenderableDesc.PipelineStateObject)
		, m_dirtyFrameCount(0)
		, m_objectsConstantBufferIndex(objectIndex)
		, m_supportsTextures(InRenderableDesc.SupportsTextures)
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
	
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const override
	{
		return m_mesh.lock()->IndexBufferView();
	}
	virtual size_t GetIndexCount() const override { return m_mesh.lock()->GetVertexIndicesCount();  }

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

	virtual bool GetSupportsTextures() const override
	{
		return m_supportsTextures;
	}

	virtual std::vector<int32_t> GetBindlessResourceIndices() const
	{
		return { m_mesh.lock()->GetVertexBufferSRV() };
	}

	virtual uint32_t GetBindlessResourceIndicesRootSignatureIndex() const
	{
		return 1; // let's go with index 1 as the bindless resource indices for the root signature entry as a convention, after the pass constants
	}

private:
	XMFLOAT4X4 m_transform;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	std::weak_ptr<IMesh> m_mesh;
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;

	// Amount of frame resources remaining that need to be updated to newer data
	int16_t m_dirtyFrameCount;

	// Index into which object constant buffer this object corresponds to
	int16_t m_objectsConstantBufferIndex;

	bool m_supportsTextures;
};