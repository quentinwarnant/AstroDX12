#pragma once

#include <Common.h>
#include <Rendering/RenderData/Mesh.h>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct RenderableObjectConstantData;

class IRenderableDesc final
{
public:
	explicit IRenderableDesc(std::weak_ptr<IMesh> inMesh, const std::wstring vsPath, std::wstring psPath,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inInputLayout,
		const XMFLOAT4X4& inInitialTransform, bool inSupportsTextures)
		: Mesh(inMesh)
		, RootSignature(nullptr)
		, VertexShaderPath(vsPath)
		, PixelShaderPath(psPath)
		, VS(nullptr)
		, PS(nullptr)
		, InputLayout(inInputLayout)
		, PipelineStateObject(nullptr)
		, InitialTransform(inInitialTransform)
		, SupportsTextures(inSupportsTextures)
	{
	}

	virtual ~IRenderableDesc() {}

	bool GetSupportsTextures()
	{
		return SupportsTextures;
	}

	std::weak_ptr<IMesh> Mesh;
	ComPtr<ID3D12RootSignature> RootSignature;

	std::wstring VertexShaderPath;
	std::wstring PixelShaderPath;
	ComPtr<IDxcBlob> VS;
	ComPtr<IDxcBlob> PS;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
	ComPtr<ID3D12PipelineState> PipelineStateObject;

	XMFLOAT4X4 InitialTransform;
	bool SupportsTextures;
};

class IRenderable
{
public:
	virtual ~IRenderable() {};

	virtual const DirectX::XMFLOAT4X4& GetWorldTransform() const = 0;
	virtual ComPtr<ID3D12RootSignature> GetGraphicsRootSignature() const = 0;
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const = 0;
	virtual size_t GetIndexCount() const = 0;
	virtual const ComPtr<ID3D12PipelineState>& GetPipelineStateObject() const = 0;
	virtual bool IsDirty() const = 0;
	virtual void MarkDirty(int16_t dirtyFrameCount) = 0;
	virtual void ReduceDirtyFrameCount() = 0;
	virtual int16_t GetConstantBufferIndex() const = 0;

	virtual std::vector<int32_t> GetBindlessResourceIndices() const = 0;
	virtual int32_t GetMeshVertexBufferSRVHeapIndex() const = 0;

	virtual uint32_t GetBindlessResourceIndicesRootSignatureIndex() const = 0;

	virtual bool GetSupportsTextures() const = 0;
};