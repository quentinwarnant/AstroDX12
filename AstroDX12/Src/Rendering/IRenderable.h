#pragma once
#include "../Common.h"
#include "RenderData/Mesh.h"

using Microsoft::WRL::ComPtr;
struct RenderableObjectConstantData;

class IRenderableDesc
{
public:
	explicit IRenderableDesc(std::unique_ptr<Mesh> inMesh, ComPtr<ID3D12RootSignature>& inRootSignature)
		: Mesh(std::move(inMesh))
		, RootSignature(inRootSignature)
		, ConstantBuffer(nullptr)
	{
	}

	std::unique_ptr<Mesh> Mesh;
	ComPtr<ID3D12RootSignature> RootSignature;
	std::unique_ptr < UploadBuffer<RenderableObjectConstantData>> ConstantBuffer;
};

class IRenderable
{
public:
	virtual ~IRenderable() {};

	virtual ComPtr<ID3D12RootSignature> GetGraphicsRootSignature() const = 0;
	virtual D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const = 0;
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const = 0;
	virtual UINT GetIndexCount() const = 0;
	virtual void SetConstantBufferData(const void* data) = 0;
};