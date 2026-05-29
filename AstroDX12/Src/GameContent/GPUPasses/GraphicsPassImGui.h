#pragma once

#include <Common.h>
#include <Rendering/Common/GPUPass.h>

class DemoManager;
class DescriptorHeap;
struct RendererContext;

class GraphicsPassImGui : public GraphicsPass
{
public:
	void Init(HWND hwnd, RendererContext& rendererContext, int numFramesInFlight, DemoManager* demoManager);

	virtual void Update(const GPUPassUpdateData& updateData) override;
	virtual void Execute(ComPtr<ID3D12GraphicsCommandList> cmdList, float deltaTime, const FrameResource& frameResources) const override;
	virtual void Shutdown() override;

private:
	void DrawDemoManagerUI();

	DemoManager* m_demoManager = nullptr;
};
