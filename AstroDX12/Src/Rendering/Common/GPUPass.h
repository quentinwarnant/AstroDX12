#pragma once

#include <Common.h>

struct FrameResource;
class DescriptorHeap;
using Microsoft::WRL::ComPtr;

enum class GPUPassType
{
	Graphics = 0,
	Compute = 1
};

class GPUPass
{
public:
	// Update resources
	virtual void Update(int32_t frameIdxModulo, void* Data) = 0;
	virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const = 0;
	virtual void OnSimReset() {}
	// Clear allocated memory
	virtual void Shutdown() = 0;

	virtual GPUPassType PassType() const = 0;
};

class GraphicsPass : public GPUPass
{
public:
	// Execute renderer logic
	virtual GPUPassType PassType() const override { return GPUPassType::Graphics; }
};

class ComputePass : public GPUPass
{
public:
	virtual GPUPassType PassType() const override { return GPUPassType::Compute; }
};
