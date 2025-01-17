#pragma once

#include <vector>
#include <string>
#include <d3d12.h>

using Microsoft::WRL::ComPtr;
struct ID3D12RootSignature;
struct ID3D12PipelineState;

class ComputableDesc
{
public:
	ComputableDesc(
		const std::wstring computeShaderPath,
		const std::vector< D3D12_INPUT_ELEMENT_DESC>& inInputLayout
	)
		: ComputeShaderPath(computeShaderPath)
		, PipelineStateObject(nullptr)
		, RootSignature(nullptr)
		, InputLayout(inInputLayout)
		, CS(nullptr)
	{
	}

	std::wstring ComputeShaderPath;

	ComPtr<ID3D12PipelineState> PipelineStateObject;
	ComPtr<ID3D12RootSignature> RootSignature;
	std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
	ComPtr<IDxcBlob> CS;
};

class IComputable
{
public:
	virtual int16_t GetObjectBufferIndex() const = 0;
	virtual ComPtr<ID3D12RootSignature> GetRootSignature() const = 0;
	virtual ComPtr<ID3D12PipelineState> GetPSO() const = 0;

};

