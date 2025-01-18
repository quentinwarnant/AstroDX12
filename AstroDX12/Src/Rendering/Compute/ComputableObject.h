#pragma once

#include <Rendering/Compute/IComputable.h>

class ComputableObject final : public IComputable
{
public:
	ComputableObject(
		ComPtr<ID3D12RootSignature>& rootSignature,
		ComPtr<ID3D12PipelineState>& pipelineStateObject,
		int16_t objectIndex
	)
		: m_rootSignature(rootSignature)
		, m_pipelineStateObject(pipelineStateObject)
		, m_objectsBufferIndex(objectIndex)
	{
	}

	virtual ~ComputableObject() {}
	
	virtual int16_t GetObjectBufferIndex() const override final
	{
		return m_objectsBufferIndex;
	}

	virtual ComPtr<ID3D12RootSignature> GetRootSignature() const override final
	{
		return m_rootSignature;
	}

	virtual ComPtr<ID3D12PipelineState> GetPSO() const override final
	{
		return m_pipelineStateObject;
	}

private:
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;

	int16_t m_objectsBufferIndex;
};

