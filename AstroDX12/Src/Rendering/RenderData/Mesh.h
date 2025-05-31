#pragma once

#include <Common.h>
#include <memory>
#include <Rendering/Common/RendererContext.h>
#include <Rendering/Common/StructuredBuffer.h>

using Microsoft::WRL::ComPtr;

class IMesh
{
public:
	IMesh(const std::string& meshName, std::vector<uint32_t> vertexIndices)
		: Name(meshName)
		, VertexIndices(vertexIndices)
		, IndexBufferByteSize(vertexIndices.size() * sizeof(std::uint32_t))
	{
	}

	virtual ~IMesh() {}

	virtual void DisposeUploadBuffers()
	{
		IndexUploadBuffer = nullptr;
	}


protected:
	std::string Name;
	
	std::vector<uint32_t> VertexIndices;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	size_t IndexBufferByteSize = 0;

	ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> IndexUploadBuffer = nullptr;

public:
	virtual D3D12_INDEX_BUFFER_VIEW IndexBufferView() const = 0;
	size_t GetVertexIndicesCount() { return VertexIndices.size(); }

	virtual int32_t GetVertexBufferSRV() const = 0;
};

// Mesh represents a lot of vertices & indices & backing memory needed to render a single mesh object
template <typename VertexDataType> //POD Data Type
class Mesh final : public IMesh
{
public: 
	Mesh(
		RendererContext& rendererContext,
		const std::string& meshName,
		std::vector<VertexDataType> vertexData,
		std::vector<uint32_t> vertexIndices)
		: IMesh(meshName, vertexIndices)
		, VertexDataStructuredBuffer(std::make_unique<StructuredBuffer<VertexDataType>>(vertexData))
	{
		// Vertex Data (structured) Buffer
		VertexDataStructuredBuffer->Init(
			rendererContext.Device.Get(),
			rendererContext.CommandList.Get(),
			true, // SRV
			false,// UAV
			*rendererContext.GlobalCBVSRVUAVDescriptorHeap.lock());

		// Vertex Index buffer
		IndexBufferGPU = AstroTools::Rendering::CreateDefaultBuffer(rendererContext.Device.Get(), rendererContext.CommandList.Get(),
			VertexIndices.data(), IndexBufferByteSize, false, IndexUploadBuffer);
	}

	Mesh(Mesh&& other) = delete;

	virtual ~Mesh() override
	{
		DisposeUploadBuffers();
	}

	std::unique_ptr<StructuredBuffer<VertexDataType>> VertexDataStructuredBuffer; // TODO: could probably store this as a IStructuredBuffer

	virtual D3D12_INDEX_BUFFER_VIEW IndexBufferView() const override
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = (UINT)IndexBufferByteSize;
		return ibv;
	}

	virtual void DisposeUploadBuffers() override
	{
		IMesh::DisposeUploadBuffers();
		VertexDataStructuredBuffer->DisposeUploadBuffer();
	}

	virtual int32_t GetVertexBufferSRV() const
	{
		return VertexDataStructuredBuffer->GetSRVIndex();
	}
};