#include "BasePassSceneGeometry.h"

#include <Common.h>
#include <Rendering/Common/FrameResource.h>
#include <Rendering/Common/UploadBuffer.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/Renderable/RenderableGroup.h>
#include <Rendering/Renderable/RenderableStaticObject.h>
#include <Rendering/RenderData/RenderConstants.h>
#include <Rendering/RenderData/VertexData.h>
#include <Rendering/RenderData/VertexDataFactory.h>
#include <Rendering/Common/VertexDataInputLayoutLibrary.h>
#include <Rendering/Common/MeshLibrary.h>
#include <Rendering/Common/ShaderLibrary.h>


using namespace AstroTools::Rendering;

void BasePassSceneGeometry::Init(IRenderer* renderer, ShaderLibrary& shaderLibrary, MeshLibrary& meshLibrary, int16_t numFrameResources)
{
    BuildSceneGeometry(renderer, meshLibrary);
    BuildShaders(shaderLibrary);
    BuildRootSignature(renderer);
    BuildPipelineStateObject(renderer);

    // We need a buffer for each frames we may have in flight, as we don't want to modify a buffer whilst it's used for rendering in another frame.
    auto BufferDataVector = std::vector<RenderableObjectConstantData>(m_renderablesDesc.size());
    for (int32_t idx = 0; idx < m_renderablesDesc.size(); ++idx)
    {
        BufferDataVector[idx].WorldTransform = m_renderablesDesc[idx].InitialTransform;
    }

    for (int16_t frameIdx = 0; frameIdx < numFrameResources; ++frameIdx)
    {
        m_renderableObjectConstantsDataBufferPerFrameResources.emplace_back(
            std::move(std::make_unique<StructuredBuffer<RenderableObjectConstantData>>(BufferDataVector)));
        renderer->CreateStructuredBufferAndViews(m_renderableObjectConstantsDataBufferPerFrameResources[frameIdx].get(), true, false);
    }

    int16_t index = 0;
    for (auto& renderableDesc : m_renderablesDesc)
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

void BasePassSceneGeometry::BuildPipelineStateObject(IRenderer* renderer)
{
    for (auto& renderableDesc : m_renderablesDesc)
    {
        renderer->CreateGraphicsPipelineState(
            renderableDesc.PipelineStateObject,
            renderableDesc.RootSignature,
            nullptr,
            renderableDesc.VS,
            renderableDesc.PS);
    }

}

SceneData BasePassSceneGeometry::LoadSceneGeometry()
{
    return SceneLoader::LoadScene(1);
}

void BasePassSceneGeometry::BuildSceneGeometry(IRenderer* renderer, MeshLibrary& meshLibrary)
{
    std::vector<VertexData_Short> verts;
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, -1.5f, -1.5f), DirectX::XMFLOAT4(Colors::White)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, +1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Black)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, +1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Red)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, -1.5f, -1.5f), DirectX::XMFLOAT4(Colors::Green)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, -1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Blue)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(-1.5f, +1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Yellow)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, +1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Cyan)));
    verts.emplace_back(VertexData_Short(DirectX::XMFLOAT3(+1.5f, -1.5f, +1.5f), DirectX::XMFLOAT4(Colors::Magenta)));

    const std::vector<std::uint32_t> indices =
    {
        // Front face
        0, 1, 2,
        0, 2, 3,

        // Back face
        4, 6, 5,
        4, 7, 6,

        // Left face
        4, 5, 1,
        4, 1, 0,

        // Right face
        3, 2, 6,
        3, 6, 7,

        // Top face
        1, 5, 6,
        1, 6, 2,

        // Bottom face
        4, 0, 3,
        4, 3, 7
    };


    const auto vertsPODList = VertexDataFactory::Convert(verts);
    auto boxMesh = meshLibrary.AddMesh(renderer->GetRendererContext(), std::string("BoxGeometry"), vertsPODList, indices);

    const auto rootPath = s2ws(DX::GetWorkingDirectory());
    const auto basicShaderPath = rootPath + std::wstring(L"\\Shaders\\basic.hlsl");
    const auto vertexColorShaderPath = rootPath + std::wstring(L"\\Shaders\\color.hlsl");
    const auto simpleNormalUVAndLightingShaderPath = rootPath + std::wstring(L"\\Shaders\\simpleNormalUVLighting.hlsl");
    const auto texturedShaderPath = rootPath + std::wstring(L"\\Shaders\\Textured.hlsl");

    // Box 1
    auto transformBox1 = XMFLOAT4X4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m_renderablesDesc.emplace_back(
        boxMesh,
        vertexColorShaderPath,
        vertexColorShaderPath,
        AstroTools::Rendering::InputLayout::IL_Pos_Color,
        transformBox1,
        false);

    // Box 2
    auto transformBox2 = XMFLOAT4X4(
        1.0f, 0.0f, 0.0f, 10.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m_renderablesDesc.emplace_back(
        boxMesh,
        vertexColorShaderPath,
        vertexColorShaderPath,
        AstroTools::Rendering::InputLayout::IL_Pos_Color,
        transformBox2,
        false);

    // Box 3
    auto transformBox3 = XMFLOAT4X4(
        4.0f, 0.0f, 0.0f, 20.0f,
        0.0f, 4.0f, 0.0f, 20.0f,
        0.0f, 0.0f, 4.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    m_renderablesDesc.emplace_back(
        boxMesh,
        vertexColorShaderPath,
        vertexColorShaderPath,
        AstroTools::Rendering::InputLayout::IL_Pos_Color,
        transformBox3,
        false);


    // .Obj load
    const auto SceneData = LoadSceneGeometry();
    for (const auto& SceneMeshObj : SceneData.SceneMeshObjects_VD_PosNormUV)
    {
        std::weak_ptr<IMesh> mesh;
        // Either find an existing mesh with the unique name or add a new one to the library
        if (!meshLibrary.GetMesh(SceneMeshObj.meshName, mesh))
        {
            const auto newMeshvertsPODList = VertexDataFactory::Convert(SceneMeshObj.verts);
            mesh = meshLibrary.AddMesh(
                renderer->GetRendererContext(),
                SceneMeshObj.meshName,
                newMeshvertsPODList,
                SceneMeshObj.indices
            );
        }

        m_renderablesDesc.emplace_back(
            mesh,
            simpleNormalUVAndLightingShaderPath,
            simpleNormalUVAndLightingShaderPath,
            AstroTools::Rendering::InputLayout::IL_Pos_Normal_UV,
            SceneMeshObj.transform,
            false);
    }
}

void BasePassSceneGeometry::BuildShaders(AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    for (auto& renderableDesc : m_renderablesDesc)
    {
        renderableDesc.VS = shaderLibrary.GetCompiledShader(renderableDesc.VertexShaderPath, L"VS", {}, L"vs_6_6");
        renderableDesc.PS = shaderLibrary.GetCompiledShader(renderableDesc.PixelShaderPath, L"PS", {}, L"ps_6_6");
    }
}

void BasePassSceneGeometry::BuildRootSignature(IRenderer* renderer)
{
    for (auto& renderableDesc : m_renderablesDesc) // TODO: move to BasePassSceneGeometry Pass 
    {
        std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

        // Descriptor table of 3 CBV:
        // one per pass,
        // one per object (transform) (object transform - to be depricated in favor of next one),
        // one for the object's bindless resource indices 

        const D3D12_ROOT_PARAMETER1 rootParamCBVPerPass
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
            .Descriptor
            {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC
            }
        };
        slotRootParams.push_back(rootParamCBVPerPass);

        const D3D12_ROOT_PARAMETER1 rootParamCBVPerObjectBindlessResourceIndices
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants
            {
                .ShaderRegister = 1,
                .RegisterSpace = 0,
                .Num32BitValues = 3
            }
        };
        slotRootParams.push_back(rootParamCBVPerObjectBindlessResourceIndices);

        //if (renderableDesc.GetSupportsTextures())
        //{
        //	CD3DX12_DESCRIPTOR_RANGE textureDescriptorRangeTable0;
        //	textureDescriptorRangeTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BINDLESS_TEXTURE2D_TABLE_SIZE, 0, TEXTURE2D_DESCRIPTOR_SPACE);
        //	slotRootParams.push_back(CD3DX12_ROOT_PARAMETER());
        //	slotRootParams[slotRootParams.size() - 1].InitAsDescriptorTable(1, & textureDescriptorRangeTable0, D3D12_SHADER_VISIBILITY_ALL);
        //}

        // Root signature is an array of root parameters
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
            (UINT)slotRootParams.size(),
            slotRootParams.data(),
            0,
            nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
        );

        // Create the root signature
        ComPtr<ID3DBlob> serializedRootSignature = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3D12SerializeVersionedRootSignature(
            &rootSignatureDesc,
            serializedRootSignature.GetAddressOf(),
            errorBlob.GetAddressOf());

        if (errorBlob)
        {
            ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        ComPtr<ID3D12RootSignature> rootSignature = nullptr;
        renderer->CreateRootSignature(serializedRootSignature, rootSignature);

        renderableDesc.RootSignature = rootSignature;
    }
}


void BasePassSceneGeometry::Update(float /*deltaTime*/, int32_t frameIdxModulo, void* /*Data*/)
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

            // TODO: use instancing correctly
            cmdList->DrawIndexedInstanced((UINT)renderableObj->GetIndexCount(), 1, 0, 0, 0);
        });
    }
}

void BasePassSceneGeometry::Shutdown() 
{

}