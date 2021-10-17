#pragma once
#include "../Common.h"

using Microsoft::WRL::ComPtr;

class IRenderable
{
public:
	virtual ~IRenderable() {};

	virtual ComPtr<ID3D12RootSignature> GetGraphicsRootSignature() const = 0;
	virtual D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const = 0;
	virtual D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const = 0;
	virtual UINT GetIndexCount() const = 0;
};