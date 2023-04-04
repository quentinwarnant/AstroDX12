#pragma once

#include <Common.h>
#include <Rendering/Common/RenderingUtils.h>

using namespace Microsoft::WRL;
using namespace DX;

class IStructuredBuffer
{
public:
	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, bool needSRV, bool needUAV, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle) = 0;
	virtual void SetHeapDescriptorIndex(int32_t descriptorIndex) = 0;
};

template<typename T>
class StructuredBuffer : public IStructuredBuffer
{
public:
	StructuredBuffer()
		: m_elementByteSize(sizeof(T))
		, m_defaultBuffer()
		, m_mappedData(nullptr)
		, m_descriptorIndex(-1)
		, m_initialised(false)
	{
		m_data = T();

		ThrowIfFailed(D3DCreateBlob(m_elementByteSize, &m_bufferCPUBlob));
		CopyMemory(m_bufferCPUBlob->GetBufferPointer(), &m_data, m_elementByteSize); // Uninitialised data at this point
	}

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, bool needSRV, bool needUAV, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle) override
	{

		m_defaultBuffer = AstroTools::Rendering::CreateDefaultBuffer(device, cmdList, &m_data, m_elementByteSize, needUAV, m_uploadBuffer);

		//const auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		//const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize);
		//ThrowIfFailed(device->CreateCommittedResource(
		//	&heapProp,
		//	D3D12_HEAP_FLAG_NONE,
		//	&bufferDesc,
		//	D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		//	nullptr,
		//	IID_PPV_ARGS(&m_defaultBuffer)
		//));

		ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));

		m_initialised = true;

		if (needSRV)
		{
			CreateSRV(device, cpuDescriptorHandle);
		}
		
		if (needUAV)
		{
			CreateUAV(device, cpuDescriptorHandle);
		}
	}

	~StructuredBuffer()
	{
		if (m_defaultBuffer)
		{
			m_defaultBuffer->Unmap(0, nullptr);
		}
		m_mappedData = nullptr;
	}

	StructuredBuffer(const StructuredBuffer& rhs) = delete;
	StructuredBuffer& operator=(const StructuredBuffer& rhs) = delete;


	ID3D12Resource* Resource() const
	{
		assert(m_initialised);
		return m_defaultBuffer.Get();
	}

	void CopyData(const T& data)
	{
		assert(m_initialised);
		memcpy(&m_mappedData, &data, sizeof(T));
	}

	UINT ByteSize()
	{
		return m_elementByteSize;
	}

	virtual void SetHeapDescriptorIndex(int32_t descriptorIndex) override
	{
		assert(m_initialised);
		m_descriptorIndex = descriptorIndex;
	}

	int32_t GetHeapDescriptorIndex()
	{
		assert(m_initialised);
		assert(m_descriptorIndex >= 0);

		return m_descriptorIndex;
	}

private:
	void CreateSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
			D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3
		);

		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = 1;
		srvDesc.Buffer.StructureByteStride = UINT(sizeof(float) * 2);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		device->CreateShaderResourceView(Resource(),  &srvDesc, cpuDescriptorHandle);
	}

	void CreateUAV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = 1;
		uavDesc.Buffer.StructureByteStride = UINT(sizeof(float) * 2);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		device->CreateUnorderedAccessView(Resource(), nullptr, &uavDesc, cpuDescriptorHandle);
	}

	UINT m_elementByteSize;
	ComPtr<ID3D12Resource> m_defaultBuffer;
	ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE* m_mappedData;
	int32_t m_descriptorIndex;
	bool m_initialised;
	ComPtr<ID3DBlob> m_bufferCPUBlob;

	T m_data;
};