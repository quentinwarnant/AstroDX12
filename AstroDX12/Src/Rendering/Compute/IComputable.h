#pragma once

#include <vector>
#include <string>
#include <d3d12.h>
#include <DXC/dxcapi.h>

using Microsoft::WRL::ComPtr;
struct ID3D12RootSignature;
struct ID3D12PipelineState;

class ComputableDesc final
{
public:
	ComputableDesc(
		const std::wstring computeShaderPath
	)
		: ComputeShaderPath(computeShaderPath)
		, PipelineStateObject(nullptr)
		, RootSignature(nullptr)
		, CS(nullptr)
	{
	}

	virtual ~ComputableDesc(){}


	std::wstring ComputeShaderPath;

	ComPtr<ID3D12PipelineState> PipelineStateObject;
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<IDxcBlob> CS;
};

class IComputable
{
public:
	virtual int16_t GetObjectBufferIndex() const = 0;
	virtual ComPtr<ID3D12RootSignature> GetRootSignature() const = 0;
	virtual ComPtr<ID3D12PipelineState> GetPSO() const = 0;
};

