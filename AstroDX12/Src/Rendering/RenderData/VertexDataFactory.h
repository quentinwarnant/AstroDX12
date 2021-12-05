#pragma once

#include <map>
#include <vector>
#include <typeindex>
#include <typeinfo>
#include "VertexData.h"

class VertexDataFactory
{
private:
	template<typename Type, typename PodType>
	[[nodiscard]] static const std::vector<PodType> Convert(const std::vector< std::unique_ptr<Type>>& inData)
	{
		auto convertedData = std::vector< PodType >(inData.size());
		uint32_t index = 0;
		for (auto& inDataInstance : inData)
		{
			convertedData[index] = *(reinterpret_cast<PodType*>(inDataInstance->GetData().get()));
			index++;
		}
		return convertedData;
	}

public:
	template<typename Type>
	static constexpr UINT GetPODTypeSize()
	{
		if (std::is_same<Type, VertexData_Short>())
		{
			return sizeof(VertexData_Short_POD);
		}

		return 0;
	}
	
	[[nodiscard]] static const std::vector<VertexData_Short_POD> Convert(const std::vector< std::unique_ptr<VertexData_Short>>& inData)
	{
		return Convert<VertexData_Short, VertexData_Short_POD >(inData);
	}


};
