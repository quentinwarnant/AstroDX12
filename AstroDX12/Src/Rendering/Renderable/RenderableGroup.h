#pragma once

#include <functional>
#include <vector>
#include <Common.h>

using Microsoft::WRL::ComPtr;
class IRenderable;
struct ID3D12PipelineState;
struct ID3D12RootSignature;

class RenderableGroup
{
public:
	RenderableGroup(const ComPtr<ID3D12PipelineState>& pso, const ComPtr<ID3D12RootSignature>& rootSignature)
		: m_pipelineStateObject(pso)
		, m_rootSignature(rootSignature)
		, m_renderables()
	{}

	~RenderableGroup()
	{
		m_renderables.clear();
	}

	void AddRenderable(std::shared_ptr<IRenderable>& renderable)
	{
		m_renderables.push_back(renderable);
	}

	ComPtr<ID3D12PipelineState> GetPSO() const
	{
		return m_pipelineStateObject;
	}

	void ForEach(std::function<void(const std::shared_ptr<IRenderable>&)> renderableIterationFn) const
	{
		for (auto& renderable : m_renderables)
		{
			renderableIterationFn(renderable);
		}
	}

	uint32_t GetRenderablesCount() const { return (uint32_t)m_renderables.size(); }

private:
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	
	std::vector<std::shared_ptr<IRenderable>> m_renderables;
};

