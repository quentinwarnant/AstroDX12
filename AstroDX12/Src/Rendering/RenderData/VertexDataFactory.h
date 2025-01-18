#pragma once

#include <map>
#include <vector>
#include <typeindex>
#include <typeinfo>

#include <Rendering/RenderData/VertexData.h>

class VertexDataFactory
{
private:
	template<typename Type, typename PodType>
	[[nodiscard]] static const std::vector<PodType> Convert(const std::vector<Type>& inData)
	{
		auto convertedData = std::vector< PodType >(inData.size());
		uint32_t index = 0;
		for (auto inDataInstance : inData)
		{
			convertedData[index] = *reinterpret_cast<PodType*>(inDataInstance.GetData().get());
			index++;
		}
		return convertedData;
	}

public:
	template<typename Type>
	static constexpr UINT GetPODTypeSize()
	{
		if (std::is_same<Type, VertexData_Pos>())
		{
			return sizeof(VertexData_Position_POD);
		}
		
		if (std::is_same<Type, VertexData_Short>())
		{
			return sizeof(VertexData_Short_POD);
		}

		if (std::is_same<Type, VertexData_Pos_Normal_UV>())
		{
			return sizeof(VertexData_Position_Normal_UV_POD);
		}

		DX::astro_assert(false, "VertexData type not supported, add entry to GetPODTypeSize");
		return 0;
	}
	
	[[nodiscard]] static const std::vector<VertexData_Position_POD> Convert( const std::vector<VertexData_Pos>& inData)
	{
		return Convert<VertexData_Pos, VertexData_Position_POD >(inData);
	}

	[[nodiscard]] static const std::vector<VertexData_Short_POD> Convert( const std::vector<VertexData_Short>& inData)
	{
		return Convert<VertexData_Short, VertexData_Short_POD >(inData);
	}

	[[nodiscard]] static const std::vector<VertexData_Position_Normal_UV_POD> Convert(const std::vector<VertexData_Pos_Normal_UV>& inData)
	{
		return Convert<VertexData_Pos_Normal_UV, VertexData_Position_Normal_UV_POD >(inData);
	}
};
