#pragma once

#include <functional>
#include <vector>
#include <Common.h>

using Microsoft::WRL::ComPtr;
class IRenderable;
struct ID3D12PipelineState;
struct ID3D12RootSignature;

// A renderable group is a array of IRenderables which share the same root signature & PSO
// TEMP: When moving to bindless resources, everything shares the same Root signature, so that should be removable, PSOs will stay bespoke to the group though
class RenderableGroup final
{
public:
	RenderableGroup(const ComPtr<ID3D12PipelineState>& pso, const ComPtr<ID3D12RootSignature>& rootSignature)
		: m_pipelineStateObject(pso)
		, m_rootSignature(rootSignature)
		, m_renderables()
	{}

	virtual ~RenderableGroup()
	{
		m_renderables.clear();
	}

	void AddRenderable(const std::shared_ptr<IRenderable>& renderable)
	{
		m_renderables.push_back(std::move(renderable));
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

	void ForEach(std::function<void(const std::shared_ptr<IRenderable>&, size_t ObjectIndex)> renderableIterationFn) const
	{
		for (size_t i = 0; i < m_renderables.size(); ++i)
		{
			renderableIterationFn(m_renderables[i],i);
		}
	}

	uint32_t GetRenderablesCount() const { return (uint32_t)m_renderables.size(); }

private:
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	
	std::vector<std::shared_ptr<IRenderable>> m_renderables;
};

