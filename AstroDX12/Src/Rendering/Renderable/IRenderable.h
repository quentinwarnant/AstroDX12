#pragma once

#include <Common.h>
#include <Rendering/RenderData/Mesh.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct RenderableObjectConstantData;

class IRenderableDesc
{
public:
	explicit IRenderableDesc(std::weak_ptr<Mesh> inMesh, const std::string vsPath, std::string psPath,
		const std::vector<D3D12_INPUT_ELEMENT_DESC>& inInputLayout,
		const XMFLOAT4X4& inInitialTransform, bool inSupportsTextures)
		: Mesh(inMesh)
		, RootSignature(nullptr)
		, VertexShaderPath( vsPath )
		, PixelShaderPath( psPath )
		, VS(nullptr)
		, PS(nullptr)
		, InputLayout(inInputLayout)
		, PipelineStateObject(nullptr)
		, InitialTransform(inInitialTransform)
		, SupportsTextures(inSupportsTextures)
	{
	}

	bool GetSupportsTextures()
	{
		return SupportsTextures;
	}

	std::weak_ptr<Mesh> Mesh;
	ComPtr<ID3D12RootSignature> RootSignature;

	std::string VertexShaderPath;
	std::string PixelShaderPath;
	ComPtr<ID3DBlob> VS;
	ComPtr<ID3DBlob> PS;

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
	virtual D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const = 0;
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const = 0;
	virtual UINT GetIndexCount() const = 0;
	virtual const ComPtr<ID3D12PipelineState>& GetPipelineStateObject() const = 0;
	virtual bool IsDirty() const = 0;
	virtual void MarkDirty(int16_t dirtyFrameCount) = 0;
	virtual void ReduceDirtyFrameCount() = 0;
	virtual int16_t GetConstantBufferIndex() const = 0;

	virtual bool GetSupportsTextures() const = 0;
};