#include "BasePassSceneGeometry.h"

#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/Renderable/RenderableStaticObject.h>
#include <Rendering/Renderable/RenderableGroup.h>
#include <Common.h>
#include <Rendering/Common/UploadBuffer.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Common/FrameResource.h>

void BasePassSceneGeometry::Init(const std::vector<IRenderableDesc>& renderablesDesc, int16_t numFrameResources)
{
    int16_t index = 0;
    for (auto& renderableDesc : renderablesDesc)
    {
        auto renderableObj = std::make_shared<RenderableStaticObject>(
            renderableDesc,
            index++
        );
        renderableObj->MarkDirty(numFrameResources);

        const auto& rootSignature = renderableObj->GetGraphicsRootSignature();
        const auto& pso = renderableObj->GetPipelineStateObject();
        RootSignaturePSOPair keyPair = { rootSignature, pso };

        auto it = m_renderableGroupMap.find(keyPair);
        if (it == m_renderableGroupMap.end())
        {
            auto emplacedItemPair = m_renderableGroupMap.emplace(keyPair, std::make_unique<RenderableGroup>(renderableObj->GetPipelineStateObject(), rootSignature));
            (*emplacedItemPair.first).second->AddRenderable( renderableObj );
        }
        else
        {
            (*it).second->AddRenderable(renderableObj);
        }
    }
}

void BasePassSceneGeometry::Update(void* Data)
{
    // re-using the same constant buffer to set all the renderables objects - per object constant data.

    auto currentFrameObjectCB  = static_cast<UploadBuffer<RenderableObjectConstantData>*>(Data);
    for (auto& [rootSignaturePSOPair, renderableGroup] : m_renderableGroupMap)
    {
        renderableGroup->ForEach([&](std::shared_ptr<IRenderable> renderable)
        {
            if (renderable->IsDirty())
            {
                XMMATRIX worldTransform = XMLoadFloat4x4(&renderable->GetWorldTransform());
                RenderableObjectConstantData objectConstants;
                XMStoreFloat4x4(&objectConstants.WorldTransform, XMMatrixTranspose(worldTransform));
                currentFrameObjectCB->CopyData(renderable->GetConstantBufferIndex(), objectConstants);

                renderable->ReduceDirtyFrameCount();
            }
        });
    }
}

void BasePassSceneGeometry::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float /*deltaTime*/, const FrameResource& frameResources) const
{
    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    for (const auto& [groupRootSignaturePsoPair, renderableGroup] : m_renderableGroupMap)
    {
        auto& renderableGroupRootSignature = groupRootSignaturePsoPair.first;

        cmdList->SetGraphicsRootSignature(renderableGroupRootSignature.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);
        cmdList->SetPipelineState(renderableGroup->GetPSO().Get());

        //TODO: add root32bit constants with indices for the shaders to read into the descriptor heap above

        renderableGroup->ForEach([&](const std::shared_ptr<IRenderable>& renderableObj, size_t ObjectIndex)
        {
            const auto renderableCBVBufferGPUAddress = frameResources.GetRenderableObjectCbvGpuAddress(ObjectIndex);
            cmdList->SetGraphicsRootConstantBufferView(2, renderableCBVBufferGPUAddress);

            //if (renderableObj->GetSupportsTextures())
            //{
            //	auto srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_renderableObjectCBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart());
            //	//TEST - really need to move this to the front of the CBV descriptor, ahead of dynamic stuff
            //	UINT srvIndex = /*amount of frame resources*/ (3 * totalRenderables) + 0;
            //	srvHandle.Offset(srvIndex, m_descriptorSizeCBV);
            //	commandList->SetGraphicsRootDescriptorTable(2, srvHandle);
            //}

            const auto indexBuffer = renderableObj->GetIndexBufferView();
            cmdList->IASetIndexBuffer(&indexBuffer);
            cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            const uint32_t BindlessResourceIndicesRootSigParamIndex = renderableObj->GetBindlessResourceIndicesRootSignatureIndex();
            const std::vector<int32_t> BindlessResourceIndices = renderableObj->GetBindlessResourceIndices();
            cmdList->SetGraphicsRoot32BitConstants(
                (UINT)BindlessResourceIndicesRootSigParamIndex,
                (UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

            cmdList->DrawIndexedInstanced((UINT)renderableObj->GetIndexCount(), 1, 0, 0, 0);
        });
    }
}

void BasePassSceneGeometry::Shutdown() 
{

}