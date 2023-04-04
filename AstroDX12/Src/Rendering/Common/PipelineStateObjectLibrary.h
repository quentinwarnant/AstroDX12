#pragma once

#include <map>
#include <External/Hash.h>
#include <type_traits>
#include <Common.h>

using namespace Microsoft::WRL;

namespace AstroTools::Rendering
{
	/*struct PSODescHashable
	{
	public:
		PSODescHashable(D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc)
		{
		
		}
	};*/


	class PipelineStateObjectLibrary
	{
	public:
		PipelineStateObjectLibrary()
			: m_cachedPSOs{}
		{}

		template<class PSOClassType>
		ComPtr<ID3D12PipelineState> GetPSO(PSOClassType& PSODesc) const
		{
			static_assert(std::is_same<PSOClassType, D3D12_GRAPHICS_PIPELINE_STATE_DESC>::value || std::is_same<PSOClassType, D3D12_COMPUTE_PIPELINE_STATE_DESC>::value);
			size_t hash = HashPSODescElements(PSODesc);
			auto it = m_cachedPSOs.find(hash);
			if (it != m_cachedPSOs.end())
			{
				return (*it).second;
			}

			return nullptr;
		}

		template<class PSOClassType>
		void CachePSO(PSOClassType& PSODesc, ComPtr<ID3D12PipelineState>& PSO)
		{
			static_assert(std::is_same<PSOClassType, D3D12_GRAPHICS_PIPELINE_STATE_DESC>::value || std::is_same<PSOClassType, D3D12_COMPUTE_PIPELINE_STATE_DESC>::value);
			m_cachedPSOs.emplace(HashPSODescElements(PSODesc), PSO);
		}

	private:
		
		size_t HashPSODescElements(const D3D12_COMPUTE_PIPELINE_STATE_DESC& PSODesc) const
		{
			size_t hash = Utility::HashState(&PSODesc.CS);
			hash = Utility::HashState(&PSODesc.Flags, 1, hash);
			hash = Utility::HashState(&PSODesc.NodeMask, 1, hash);
			hash = Utility::HashState(&PSODesc.CachedPSO, 1, hash);
			return hash;
		}

		size_t HashPSODescElements(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc) const
		{
			size_t hash = Utility::HashState(&PSODesc.VS);
			hash = Utility::HashState(&PSODesc.PS,1,hash);
			hash = Utility::HashState(&PSODesc.DS, 1, hash);
			hash = Utility::HashState(&PSODesc.HS, 1, hash);
			hash = Utility::HashState(&PSODesc.GS, 1, hash);
			hash = Utility::HashState(&PSODesc.StreamOutput, 1, hash);
			hash = Utility::HashState(&PSODesc.BlendState, 1, hash);
			hash = Utility::HashState(&PSODesc.SampleMask, 1, hash);
			hash = Utility::HashState(&PSODesc.RasterizerState, 1, hash);
			hash = Utility::HashState(&PSODesc.DepthStencilState, 1, hash);
			for (UINT inputLayoutIdx = 0; inputLayoutIdx < PSODesc.InputLayout.NumElements; ++inputLayoutIdx)
			{
				hash = Utility::HashState(&PSODesc.InputLayout.pInputElementDescs[inputLayoutIdx], 1, hash);
			}
			hash = Utility::HashState(&PSODesc.IBStripCutValue, 1, hash);
			hash = Utility::HashState(&PSODesc.PrimitiveTopologyType, 1, hash);
			hash = Utility::HashState(&PSODesc.NumRenderTargets, 1, hash);

			for (UINT RTVFormatIdx = 0; RTVFormatIdx < 8; ++RTVFormatIdx)
			{
				hash = Utility::HashState(&PSODesc.RTVFormats[RTVFormatIdx], 1, hash);
			}
			hash = Utility::HashState(&PSODesc.DSVFormat, 1, hash);
			hash = Utility::HashState(&PSODesc.SampleDesc, 1, hash);
			hash = Utility::HashState(&PSODesc.NodeMask, 1, hash);
			hash = Utility::HashState(&PSODesc.CachedPSO, 1, hash);
			hash = Utility::HashState(&PSODesc.Flags, 1, hash);

			return hash;
		}

		//Key is the hash of the PSO Desc
		std::map<size_t, ComPtr<ID3D12PipelineState>> m_cachedPSOs;
	};
}