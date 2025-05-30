#include "ComputePassAddBufferValues.h"
#include <Rendering/Common/FrameResource.h>
#include <Rendering/Common/DescriptorHeap.h>

void ComputePassAddBufferValues::Init(const std::vector<ComputableDesc>& computableDescs)
{
	int16_t index = 0;
	for (auto& computableDesc : computableDescs)
	{
		m_computeGroup->Computables.emplace_back(std::move(std::make_shared<ComputableObject>(
			computableDesc.RootSignature,
			computableDesc.PipelineStateObject,
			index)));
		index++;
	}
}

void ComputePassAddBufferValues::Update(void* /*Data*/)
{

}

void ComputePassAddBufferValues::Execute(
	ComPtr<ID3D12GraphicsCommandList> cmdList,
	float /*deltaTime*/,
	const FrameResource& frameResources,
	const DescriptorHeap& descriptorHeap,
	int32_t descriptorSizeCBV) const
{
	m_computeGroup->ForEach([&](const std::shared_ptr<IComputable>& computableObj)
	{
		cmdList->SetComputeRootSignature(computableObj->GetRootSignature().Get());
		cmdList->SetPipelineState(computableObj->GetPSO().Get());

		UINT bufferIndex = computableObj->GetObjectBufferIndex() * 3;
		// Buffer SRV 1
		cmdList->SetComputeRootShaderResourceView(0, frameResources.ComputableObjectStructuredBufferPerObj[bufferIndex]->Resource()->GetGPUVirtualAddress());
		// Buffer SRV 2
		cmdList->SetComputeRootShaderResourceView(1, frameResources.ComputableObjectStructuredBufferPerObj[bufferIndex + 1]->Resource()->GetGPUVirtualAddress());
		// Buffer UAV 1
		cmdList->SetComputeRootUnorderedAccessView(2, frameResources.ComputableObjectStructuredBufferPerObj[bufferIndex + 2]->Resource()->GetGPUVirtualAddress());

		cmdList->Dispatch(1, 1, 1);
	});
}

void ComputePassAddBufferValues::Shutdown()
{

}
