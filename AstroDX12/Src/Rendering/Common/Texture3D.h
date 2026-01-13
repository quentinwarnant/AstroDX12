#pragma once

#include <Common.h>
#include <Rendering/Common/DescriptorHeap.h>
#include <Rendering/Common/RendererContext.h>
#include <Rendering/Common/RenderingUtils.h>

using namespace Microsoft::WRL;

class ITexture3D
{
public:
	virtual void Init(
		RendererContext& rendererContext,
		bool needUAV,
		DescriptorHeap& gpuVisibleDescriptorHeap,
		DescriptorHeap& cpuVisibleDescriptorHeap,
		std::wstring name,
		DXGI_FORMAT format,
		int32_t width,
		int32_t height,
		int32_t depth,
		int16_t mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN
		) = 0;
	
};


class Texture3D : public ITexture3D
{
public:
	Texture3D()
		: m_texture3DResource(nullptr)
	{
	}

	virtual ~Texture3D()
	{
		if (m_texture3DResource.Get())
		{
			m_texture3DResource->Release();
		}
	}

	virtual void Init(
		RendererContext& rendererContext,
		bool needUAV, 
		DescriptorHeap& gpuVisibleDescriptorHeap,
		DescriptorHeap& cpuVisibleDescriptorHeap,
		std::wstring name,
		DXGI_FORMAT format, 
		int32_t width,
		int32_t height,
		int32_t depth,
		int16_t mipLevels = 0,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN
		) override
	{
		m_texture3DResource = AstroTools::Rendering::CreateTexture3D(
			rendererContext.Device.Get(),
			needUAV,
			format,
			width,
			height,
			depth,
			mipLevels,
			flags,
			layout			
		);

		m_texture3DResource->SetName( name.c_str() );

		// Create UAV and SRV
		if (needUAV)
		{
			m_uavIndex = gpuVisibleDescriptorHeap.GetCurrentDescriptorHeapHandle();

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = -1;

			//m_uavDescriptorHandle = gpuVisibleDescriptorHeap.GetGPUDescriptorHandleByIndex(m_uavIndex);
			auto uavCPUDescriptorHandle = cpuVisibleDescriptorHeap.GetCPUDescriptorHandleByIndex(m_uavIndex);
			rendererContext.Device->CreateUnorderedAccessView(
				m_texture3DResource.Get(),
				nullptr,
				&uavDesc,
				uavCPUDescriptorHandle);

			rendererContext.Device->CopyDescriptorsSimple(1, gpuVisibleDescriptorHeap.GetCPUDescriptorHandleByIndex(m_uavIndex), uavCPUDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			gpuVisibleDescriptorHeap.IncreaseCurrentDescriptorHeapHandle();
		}

		m_srvIndex = gpuVisibleDescriptorHeap.GetCurrentDescriptorHeapHandle();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.MipLevels = 1;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.f;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		//auto srvDescriptorHandle = gpuVisibleDescriptorHeap.GetGPUDescriptorHandleByIndex(m_srvIndex);
		rendererContext.Device->CreateShaderResourceView(
			m_texture3DResource.Get(),
			&srvDesc,
			gpuVisibleDescriptorHeap.GetCPUDescriptorHandleByIndex(m_srvIndex));

		gpuVisibleDescriptorHeap.IncreaseCurrentDescriptorHeapHandle();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture3DResource;
	int32_t m_uavIndex = -1;
	int32_t m_srvIndex = -1;
};