#pragma once

#include <Common.h>
#include <Rendering/Common/DescriptorHeap.h>
#include <Rendering/Common/RenderingUtils.h>
#include <vector>

using namespace Microsoft::WRL;
using namespace DX;

class IStructuredBuffer
{
public:
	virtual void Init(
		ID3D12Device* device, 
		ID3D12GraphicsCommandList* cmdList,
		std::wstring_view bufferName, 
		bool needSRV,
		bool needUAV, 
		DescriptorHeap& descriptorHeap, 
		bool viewsAreByteAddress = false) = 0;
	virtual void DisposeUploadBuffer() = 0;
};

template<typename T>
class StructuredBuffer : public IStructuredBuffer
{
public:
	StructuredBuffer(std::vector<T> bufferData)
		: m_elementByteSize(sizeof(T))
		, m_defaultBuffer()
		, m_mappedData(nullptr)
		, SrvIndex(-1)
		, UavIndex(-1)
		, m_initialised(false)
	{
		m_dataVector = std::move(bufferData);
	}

	virtual ~StructuredBuffer()
	{
		if (m_uploadBuffer)
		{
			m_uploadBuffer->Unmap(0, nullptr);
		}

		m_mappedData = nullptr;
	}

	virtual void Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::wstring_view bufferName, bool needSRV, bool needUAV, DescriptorHeap& descriptorHeap, bool viewsAreByteAddress = false) override
	{
		// Creates both he default buffer and the "helper" upload buffer + adds the copy of data resources copy to the command list
		m_defaultBuffer = AstroTools::Rendering::CreateDefaultBuffer(
			device,
			cmdList,
			m_dataVector.data(),
			m_dataVector.size() * m_elementByteSize,
			needUAV,
			bufferName,
			m_uploadBuffer);

		// Assigns a pointer to m_mappedData that points to the subresource 0 inside the "uploadBuffer" I3D12Resource
		// This way we can modify the contents of the upload buffer using CopyData()
		// TODO: But somewhere we'll presumably still need to force a copy of subresource from the upload buffer to the default buffer (post initialisation if we modify it at runtime)
		ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));

		m_initialised = true;

		if (needSRV)
		{
			SrvIndex = descriptorHeap.GetCurrentDescriptorHeapHandle();
			if (viewsAreByteAddress)
			{
				CreateByteAddressSRV(device, descriptorHeap.GetCPUDescriptorHandleByIndex(SrvIndex));
			}
			else
			{
				CreateSRV(device, descriptorHeap.GetCPUDescriptorHandleByIndex(SrvIndex));
			}
			descriptorHeap.IncreaseCurrentDescriptorHeapHandle();
		}
		
		if (needUAV)
		{
			UavIndex = descriptorHeap.GetCurrentDescriptorHeapHandle();
			if (viewsAreByteAddress)
			{
				CreateByteAddressUAV(device, descriptorHeap.GetCPUDescriptorHandleByIndex(UavIndex));
			}
			else
			{
				CreateUAV(device, descriptorHeap.GetCPUDescriptorHandleByIndex(UavIndex));
			}
			descriptorHeap.IncreaseCurrentDescriptorHeapHandle();
		}
	}

	StructuredBuffer(const StructuredBuffer& rhs) = delete;
	StructuredBuffer& operator=(const StructuredBuffer& rhs) = delete;

	ID3D12Resource* Resource() const
	{
		assert(m_initialised);
		return m_defaultBuffer.Get();
	}

	void CopyData(const std::vector<T>& data)
	{
		assert(m_initialised);
		memcpy(&m_mappedData, data.data(), sizeof(T) * data.size());
		//TODO: schedule copy of subresource in uploadbuffer to common buffer (using cmd list) to push the data to fast GPU visible memory
	}

	UINT ByteSize()
	{
		return m_elementByteSize;
	}

	// returns the descriptor index of an element in the descriptor heap this resource has a SRV for
	int32_t GetSRVIndex() const 
	{
		return SrvIndex;
	}

	// returns the descriptor index of an element in the descriptor heap this resource has a SRV for
	int32_t GetUAVIndex() const 
	{
		DX::astro_assert(UavIndex!= -1, "UAV Index is invalid, UAV hasn't been initialised");

		return UavIndex;
	}

	virtual void DisposeUploadBuffer() override
	{
		m_uploadBuffer = nullptr;
	}

private:
	void CreateSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = UINT(m_dataVector.size());
		srvDesc.Buffer.StructureByteStride = m_elementByteSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		device->CreateShaderResourceView(Resource(),  &srvDesc, cpuDescriptorHandle);
	}

	void CreateByteAddressSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = UINT(sizeof(m_dataVector[0]) * m_dataVector.size() / 4); // elements for byteAddressBuffer is measured in number of 4 bytes
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		device->CreateShaderResourceView(Resource(), &srvDesc, cpuDescriptorHandle);
	}

	void CreateUAV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = UINT(m_dataVector.size());
		uavDesc.Buffer.StructureByteStride = m_elementByteSize;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		device->CreateUnorderedAccessView(Resource(), nullptr, &uavDesc, cpuDescriptorHandle);
	}

	void CreateByteAddressUAV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = UINT(sizeof(m_dataVector[0]) * m_dataVector.size() / 4); // elements for byteAddressBuffer is measured in number of 4 bytes
		uavDesc.Buffer.StructureByteStride = 0;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

		device->CreateUnorderedAccessView(Resource(), nullptr, &uavDesc, cpuDescriptorHandle);
	}

	UINT m_elementByteSize{ -1 };
	ComPtr<ID3D12Resource> m_defaultBuffer{ nullptr };
	ComPtr<ID3D12Resource> m_uploadBuffer{ nullptr };
	BYTE* m_mappedData{ nullptr };
	int32_t SrvIndex{-1};
	int32_t UavIndex{ -1 };
	bool m_initialised{ false };

	std::vector<T> m_dataVector{};
};