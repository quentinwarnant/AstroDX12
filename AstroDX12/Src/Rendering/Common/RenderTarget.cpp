#include "RenderTarget.h"

#include <Rendering/IRenderer.h>
#include <Rendering/Common/RenderingUtils.h>
#include <Rendering/Common/DescriptorHeap.h>
#include <Rendering/Common/RendererContext.h>

void RenderTarget::Initialize(IRenderer* renderer,
	DescriptorHeap& descriptorHeap, 
	LPCWSTR name,
	UINT32 width,
	UINT32 height,
	DXGI_FORMAT format, 
	D3D12_RESOURCE_STATES initialState)
{
	m_width = width;
	m_height = height;
	m_format = format;

	m_renderTargetResource = AstroTools::Rendering::CreateRenderTarget(
		renderer->GetRendererContext().Device.Get(),
		m_width, m_height, m_format, initialState);

	m_renderTargetResource->SetName(name);

	m_uavIndex = descriptorHeap.GetCurrentDescriptorHeapHandle();
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	m_uavDescriptorHandle = descriptorHeap.GetGPUDescriptorHandleByIndex(m_uavIndex);
	renderer->GetRendererContext().Device->CreateUnorderedAccessView(
		m_renderTargetResource.Get(),
		nullptr,
		&uavDesc,
		descriptorHeap.GetCPUDescriptorHandleByIndex(m_uavIndex));

	descriptorHeap.IncreaseCurrentDescriptorHeapHandle();

	m_srvIndex = descriptorHeap.GetCurrentDescriptorHeapHandle();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_srvDescriptorHandle = descriptorHeap.GetGPUDescriptorHandleByIndex(m_srvIndex);
	renderer->GetRendererContext().Device->CreateShaderResourceView(
		m_renderTargetResource.Get(),
		&srvDesc,
		descriptorHeap.GetCPUDescriptorHandleByIndex(m_srvIndex));

	descriptorHeap.IncreaseCurrentDescriptorHeapHandle();
}

void RenderTarget::ReleaseResources()
{
	if (m_renderTargetResource)
	{
		m_renderTargetResource = nullptr;
	}
	m_uavIndex = -1;
}
