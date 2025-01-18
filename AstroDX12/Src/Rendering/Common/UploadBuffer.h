#pragma once

#include <Common.h>
#include <Rendering/Common/RenderingUtils.h>

using namespace Microsoft::WRL;
using namespace DX;

template<typename T>
class UploadBuffer final
{
public: 
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer)
		: m_isConstantBuffer(isConstantBuffer)
		, m_elementByteSize(sizeof(T))
	{
		// Constant buffer elements need to be multiples of 256 bytes.
		// This is because the hardware can only view constant data 
		// at m*256 byte offsets and of n*256 byte lengths. 
		if (isConstantBuffer)
		{
			m_elementByteSize = AstroTools::Rendering::CalcConstantBufferByteSize(sizeof(T));
		}

		const auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * elementCount);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_uploadBuffer)
		));

		ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
	}

	virtual ~UploadBuffer()
	{
		if (m_uploadBuffer) 
		{
			m_uploadBuffer->Unmap(0, nullptr);
		}
		m_mappedData = nullptr;
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

	ID3D12Resource* Resource() const
	{
		return m_uploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&m_mappedData[elementIndex * m_elementByteSize], &data, sizeof(T));
	}

	UINT GetElementByteSize() { return m_elementByteSize; }

private:
	bool m_isConstantBuffer;
	UINT m_elementByteSize;

	ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE* m_mappedData;
};