#pragma once

#include <Common.h>

#include <Rendering/Compute/ComputableObject.h>
#include <functional>

using Microsoft::WRL::ComPtr;

class ComputeGroup
{
public:
	ComputeGroup()
		:Computables()
	{}

	std::vector<std::shared_ptr<ComputableObject>> Computables;

	void ForEach(std::function<void(const std::shared_ptr<IComputable>&)> computableIterationFn) const
	{
		for (const auto& computable : Computables)
		{
			computableIterationFn(computable);
		}
	}
};

