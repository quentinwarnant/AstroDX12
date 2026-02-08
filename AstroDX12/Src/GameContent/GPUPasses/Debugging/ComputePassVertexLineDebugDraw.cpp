#include "ComputePassVertexLineDebugDraw.h"
#include <Rendering/IRenderer.h>
#include <Rendering/Common/ShaderLibrary.h>
#include <Rendering/Common/FrameResource.h>
#include <memory>

// This pass will make 2 dispatches:
// 1. One to configure the indirect draw argument buffer based on the number of lines to draw.
// 2. One to execute the indirect draw

namespace Privates
{
	std::size_t MaxDebugLines = 10'000'000;
}

void ComputePassVertexLineDebugDraw::Init(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
    auto BufferDataVector = std::vector<DebugVertexLineData>(Privates::MaxDebugLines);
    
	// Buffer to store the debug line data into
    m_debugDataBuffer = std::make_unique<StructuredBuffer<DebugVertexLineData>>(BufferDataVector);
    renderer->CreateStructuredBufferAndViews(m_debugDataBuffer.get(), std::wstring_view(L"LineDebugDrawData"), true, true);

	// Buffer to count the number of lines to draw
	std::vector< uint32_t > lineCounterData(1);
	m_lineCountBuffer = std::make_unique<StructuredBuffer<uint32_t>>(lineCounterData);
	renderer->CreateStructuredBufferAndViews(m_lineCountBuffer.get(), std::wstring_view(L"LineDebugDrawCount"), true, true, /*byteAddressBuffer*/true);

	// Buffer to store the indirect draw args into 
	std::vector<DrawIndirectArgs> drawArgsData(1);
	m_indirectArgsBuffer = std::make_unique<StructuredBuffer<DrawIndirectArgs>>(drawArgsData);
	renderer->CreateStructuredBufferAndViews(m_indirectArgsBuffer.get(), std::wstring_view(L"LineDebugDrawIndirectArgs"), true, true);


	// TODO: set resources in the correct initial state?


    CreateRootSignatures(renderer);
    CreatePipelineState(renderer, shaderLibrary);

}

void ComputePassVertexLineDebugDraw::CreateRootSignatures(IRenderer* renderer)
{
	// ComputeIndirectArgs
	{
		std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

		const D3D12_ROOT_PARAMETER1 rootParamBindlessResourceIndices
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants
			{
				.ShaderRegister = 0,
				.RegisterSpace = 0,
				.Num32BitValues = 4
			}
		};
		slotRootParams.push_back(rootParamBindlessResourceIndices);

		// Root signature is an array of root parameters
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			(UINT)slotRootParams.size(),
			slotRootParams.data(),
			0,
			nullptr,
			//D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
		);

		// Create the root signature
		ComPtr<ID3DBlob> serializedRootSignature = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			serializedRootSignature.GetAddressOf(),
			errorBlob.GetAddressOf());

		if (errorBlob)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		renderer->CreateRootSignature(serializedRootSignature, m_rsComputeIndirectArgs);
	}

	// Draw
	{
		std::vector<D3D12_ROOT_PARAMETER1> slotRootParams;

		const D3D12_ROOT_PARAMETER1 rootParamBindlessResourceIndices
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants
			{
				.ShaderRegister = 0,
				.RegisterSpace = 0,
				.Num32BitValues = 2
			}
		};
		slotRootParams.push_back(rootParamBindlessResourceIndices);

		// Root signature is an array of root parameters
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			(UINT)slotRootParams.size(),
			slotRootParams.data(),
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
		);

		// Create the root signature
		ComPtr<ID3DBlob> serializedRootSignature = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			serializedRootSignature.GetAddressOf(),
			errorBlob.GetAddressOf());

		if (errorBlob)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		renderer->CreateRootSignature(serializedRootSignature, m_rsDrawDebug);
	}

	// DrawIndirect
	{
		std::vector< D3D12_INDIRECT_ARGUMENT_DESC> indirectArgDescs;
		// This will be copied to the root parameter at index 0 in the root indirectly executed signature (which is expected to be the bindless buffer view indices)
		D3D12_INDIRECT_ARGUMENT_DESC argDesc0 = {};
		argDesc0.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		argDesc0.Constant.RootParameterIndex = 0;
		argDesc0.Constant.DestOffsetIn32BitValues = 0;
		argDesc0.Constant.Num32BitValuesToSet = 2;
		indirectArgDescs.push_back(argDesc0);

		D3D12_INDIRECT_ARGUMENT_DESC argDesc2 = {};
		argDesc2.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
		indirectArgDescs.push_back(argDesc2);

		D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
		sigDesc.ByteStride = sizeof(DrawIndirectArgs);
		sigDesc.NumArgumentDescs = indirectArgDescs.size();
		sigDesc.pArgumentDescs = indirectArgDescs.data();

		renderer->CreateCommandSignature(&sigDesc, m_rsDrawDebug, m_rsDispatchIndirectDraw);
	}

}

void ComputePassVertexLineDebugDraw::CreatePipelineState(IRenderer* renderer, AstroTools::Rendering::ShaderLibrary& shaderLibrary)
{
	{
		const std::wstring shaderPath = L"Shaders/VertexLineDebugDrawComputeIndirectArgs.hlsl";
		auto CS = shaderLibrary.GetCompiledShader(shaderPath, L"CS", {}, L"cs_6_6");

		renderer->CreateComputePipelineState(
			m_psoComputeIndirectArgs,
			m_rsComputeIndirectArgs,
			CS);
	}

	{
		const std::wstring shaderPath = L"Shaders/VertexLineDebugDraw.hlsl";
		auto VS = shaderLibrary.GetCompiledShader(shaderPath, L"VS", {}, L"vs_6_6");
		auto PS = shaderLibrary.GetCompiledShader(shaderPath, L"PS", {}, L"ps_6_6");

		renderer->CreateGraphicsPipelineState(
			m_psoDrawDebug,
			m_rsDrawDebug,
			nullptr,
			VS,
			PS,
			false,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE); // Line primitive!
	}
}

void ComputePassVertexLineDebugDraw::Update(const GPUPassUpdateData& /*updateData*/)
{
}

void ComputePassVertexLineDebugDraw::Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float /*deltaTime*/, const FrameResource& frameResources) const
{
	ExecuteComputeIndirectArgs(cmdList, frameResources);
	ExecuteIndirectDraw(cmdList, frameResources);
}

void ComputePassVertexLineDebugDraw::ExecuteComputeIndirectArgs(ComPtr<ID3D12GraphicsCommandList> cmdList, const FrameResource& frameResources) const
{
	// Calculate indirect draw args - by dispatching compute shader
	PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "ComputePassVertexLineDebugDraw::ExecuteComputeIndirectArgs");

	// Transition buffers into correct state for the compute indirect args dispatch
	// Transition resources into their next correct state

	std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransitions;

	// Indirect args buffer becomes UAV
	buffersStateTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
		m_indirectArgsBuffer->Resource(),
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	cmdList->ResourceBarrier((UINT)buffersStateTransitions.size(), buffersStateTransitions.data());

	// Dispatch compute shader to calculate the indirect draw args
	cmdList->SetComputeRootSignature(m_rsComputeIndirectArgs.Get());
	cmdList->SetPipelineState(m_psoComputeIndirectArgs.Get());

	constexpr int32_t BindlessResourceIndicesRootSigParamIndex = 0;
	const std::vector<int32_t> BindlessResourceIndices = {
		m_lineCountBuffer->GetUAVIndex(),
		m_indirectArgsBuffer->GetUAVIndex(),
		m_debugDataBuffer->GetSRVIndex(),
		frameResources.PassConstantBuffer->GetHeapDescriptorIndex()
	};

	cmdList->SetComputeRoot32BitConstants(
		(UINT)BindlessResourceIndicesRootSigParamIndex,
		(UINT)BindlessResourceIndices.size(), BindlessResourceIndices.data(), 0);

	cmdList->Dispatch(1, 1, 1);

	// Transition buffers back to correct state for the indirect draw & to be ready for the next compute shader dispatches
	buffersStateTransitions.clear();

	// Indirect args buffer becomes SRV
	buffersStateTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
		m_indirectArgsBuffer->Resource(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT));

	cmdList->ResourceBarrier(buffersStateTransitions.size(), buffersStateTransitions.data());
}

void ComputePassVertexLineDebugDraw::ExecuteIndirectDraw(ComPtr<ID3D12GraphicsCommandList> cmdList, const FrameResource& /*frameResources*/) const
{
	// Calculate indirect draw args - by dispatching compute shader
	PIXScopedEvent(cmdList.Get(), PIX_COLOR(255, 128, 0), "ComputePassVertexLineDebugDraw::ExecuteIndirectDraw");

	
	std::vector<CD3DX12_RESOURCE_BARRIER> buffersStateTransitions;
	// debug line draw buffer becomes SRV
	buffersStateTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
		m_debugDataBuffer->Resource(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	cmdList->ResourceBarrier((UINT)buffersStateTransitions.size(), buffersStateTransitions.data());

	cmdList->SetPipelineState(m_psoDrawDebug.Get());
	cmdList->SetGraphicsRootSignature(m_rsDrawDebug.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	// TODO: use bindless CBV index
	//const auto frameResourceCBVBufferGPUAddress = frameResources.PassConstantBuffer->Resource()->GetGPUVirtualAddress();
	//cmdList->SetGraphicsRootConstantBufferView(1, frameResourceCBVBufferGPUAddress);

	cmdList->ExecuteIndirect(
		m_rsDispatchIndirectDraw.Get(),
		1,									// Max command count
		m_indirectArgsBuffer->Resource(),   // Buffer containing the draw args
		0,									// Offset
		nullptr,							// Count buffer (not needed for single draw)
		0);

	// Reset to triangle list for any subsequent draw calls that might expect it as a default
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// debug line draw buffer becomes UAV for the next compute shader dispatch that will write to it
	buffersStateTransitions.clear();
	buffersStateTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
		m_debugDataBuffer->Resource(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	cmdList->ResourceBarrier((UINT)buffersStateTransitions.size(), buffersStateTransitions.data());
}

void ComputePassVertexLineDebugDraw::Shutdown()
{
}