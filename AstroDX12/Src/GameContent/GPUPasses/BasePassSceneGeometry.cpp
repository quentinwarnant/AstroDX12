#include "BasePassSceneGeometry.h"

#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/Renderable/RenderableStaticObject.h>
#include <Rendering/Renderable/RenderableGroup.h>
#include <Common.h>
#include <Rendering/Common/UploadBuffer.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Common/FrameResource.h>


void BasePassSceneGeometry::Init(IRenderer* renderer, const std::vector<IRenderableDesc>& renderablesDesc, int16_t numFrameResources)
{
    // We need a buffer for each frames we may have in flight, as we don't want to modify a buffer whilst it's used for rendering in another frame.
    auto BufferDataVector = std::vector<RenderableObjectConstantData>(renderablesDesc.size());
    for (int32_t idx = 0; idx < renderablesDesc.size(); ++idx)
    {
        BufferDataVector[idx].WorldTransform = renderablesDesc[idx].InitialTransform;
    }

    for (int16_t frameIdx = 0; frameIdx < numFrameResources; ++frameIdx)
    {
        m_renderableObjectConstantsDataBufferPerFrameResources.emplace_back(
            std::move(std::make_unique<StructuredBuffer<RenderableObjectConstantData>>(BufferDataVector)));
        renderer->CreateStructuredBufferAndViews(m_renderableObjectConstantsDataBufferPerFrameResources[frameIdx].get(), true, false);
    }

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

void BasePassSceneGeometry::Update(int32_t frameIdxModulo, void* /*Data*/)
{
    m_frameIdxModulo = frameIdxModulo;
    // re-using the same constant buffer to set all the renderables objects - per object constant data.

    //TODO: re-enable this with the new 1 buffer format - by allowing to do partial updates to the structured buffer type (currently it's not possible)
    
    //auto& currentFrameObjectConstantsDataBuffer = *m_renderableObjectConstantsDataBufferPerFrameResources[frameIdxModulo].get();
    //for (auto& [rootSignaturePSOPair, renderableGroup] : m_renderableGroupMap)
    //{
    //    renderableGroup->ForEach([&](std::shared_ptr<IRenderable> renderable)
    //    {
    //        if (renderable->IsDirty())
    //        {
    //            //TODO: need to understand better how buffer uploads are processed
    //            //TODO: then allow partial buffer updates for only the dirty objects's data

    //            XMMATRIX worldTransform = XMLoadFloat4x4(&renderable->GetWorldTransform());
    //            RenderableObjectConstantData objectConstants;
    //            XMStoreFloat4x4(&objectConstants.WorldTransform, XMMatrixTranspose(worldTransform));
    //            currentFrameObjectConstantsDataBuffer->CopyData(renderable->GetConstantBufferIndex(), objectConstants);

    //            renderable->ReduceDirtyFrameCount();
    //        }
    //    });
    //}
}

void BasePassSceneGeometry::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float /*deltaTime*/, const FrameResource& frameResources) const
{
    PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "BasePassSceneGeometry");

    auto& currentFrameObjectConstantsDataBuffer = *m_renderableObjectConstantsDataBufferPerFrameResources[m_frameIdxModulo].get();

    const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
    for (const auto& [groupRootSignaturePsoPair, renderableGroup] : m_renderableGroupMap)
    {
        auto& renderableGroupRootSignature = groupRootSignaturePsoPair.first;

        cmdList->SetGraphicsRootSignature(renderableGroupRootSignature.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, frameResourceCBVBufferGPUAddress);
        cmdList->SetPipelineState(renderableGroup->GetPSO().Get());

        renderableGroup->ForEach([&](const std::shared_ptr<IRenderable>& renderableObj, size_t ObjectIndex)
        {
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
            const std::vector<int32_t> BindlessResourceIndices
            {
                renderableObj->GetMeshVertexBufferSRVHeapIndex(),
                currentFrameObjectConstantsDataBuffer.GetSRVIndex(),
                int32_t(ObjectIndex)
            };
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