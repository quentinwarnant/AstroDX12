#pragma once

#include <Common.h>
#include <Rendering/RenderData/Mesh.h>

using Microsoft::WRL::ComPtr;
struct RenderableObjectConstantData;

class IRenderableDesc
{
public:
	explicit IRenderableDesc(std::unique_ptr<Mesh> inMesh, const std::string vsPath, std::string psPath , const std::vector<D3D12_INPUT_ELEMENT_DESC>& inInputLayout)
		: Mesh(std::move(inMesh))
		, RootSignature(nullptr)
		, ConstantBuffer(nullptr)
		, VertexShaderPath( vsPath )
		, PixelShaderPath( psPath )
		, VS(nullptr)
		, PS(nullptr)
		, InputLayout(inInputLayout)
		, PipelineStateObject(nullptr)
	{
	}

	std::unique_ptr<Mesh> Mesh;
	ComPtr<ID3D12RootSignature> RootSignature;
	std::unique_ptr<UploadBuffer<RenderableObjectConstantData>> ConstantBuffer;

	std::string VertexShaderPath;
	std::string PixelShaderPath;
	ComPtr<ID3DBlob> VS;
	ComPtr<ID3DBlob> PS;

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
	ComPtr<ID3D12PipelineState> PipelineStateObject;
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
	virtual ComPtr<ID3D12PipelineState> GetPipelineStateObject() const = 0;
};